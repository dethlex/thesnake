#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <random>

using namespace sf;

static const int M = 10, N = 20;
static const int STEP = 18;

enum class Direction
{
	NONE, UP, LEFT, DOWN, RIGHT
};

class GameObject
{
protected:
	Sprite tile_;
	Vector2i pos_;

public:
	explicit GameObject(const Texture & texture)
	{
		tile_.setTexture(texture);
	}

	const Sprite & GetTile() const
	{
		return tile_;
	}

	const Vector2i & GetPosition() const
	{
		return pos_;
	}

};

class Body: public GameObject
{
private:
	Direction direction_ = Direction::NONE;

public:
	explicit Body(const Texture & texture, const Vector2i & pos) : GameObject(texture)
	{
		pos_ = pos;
		tile_.setTextureRect(IntRect(0, 0, STEP, STEP));
	}

	Direction GetDirection() const
	{
		return direction_;
	}

	// возвращаем предыдущее направление
	Direction Move(Direction dir)
	{
		auto prev_dir = direction_;

		if (dir != Direction::NONE)
			direction_ = dir;

		switch (direction_)
		{
			case Direction::UP:
				pos_.y -= 1;
				break;
			case Direction::DOWN:
				pos_.y += 1;
				break;
			case Direction::LEFT:
				pos_.x -= 1;
				break;
			case Direction::RIGHT:
				pos_.x += 1;
				break;
			default: ;
		}

		tile_.setPosition(static_cast<Vector2f>(pos_ * STEP));

		return prev_dir;
	}

};

class Snake : private std::vector<Body>
{
private:
	Body tail_;
	int score_ = 0;

private:
	void CheckDirection(Direction & dir) const
	{
		if (dir == Head().GetDirection() ||
			(dir == Direction::UP && Head().GetDirection() == Direction::DOWN)    ||
			(dir == Direction::DOWN && Head().GetDirection() == Direction::UP)    ||
			(dir == Direction::RIGHT && Head().GetDirection() == Direction::LEFT) ||
			(dir == Direction::LEFT && Head().GetDirection() == Direction::RIGHT)  )
		{
			dir = Direction::NONE;
		}
	}

	const Body & Head() const
	{
		return *begin();
	}

public:
	Snake(std::initializer_list<Body> bodies) : std::vector<Body>(bodies), tail_(back()) {}

	bool CheckIntersects() const
	{
		if (Head().GetPosition().y < 0  ||
		    Head().GetPosition().y >= N ||
		    Head().GetPosition().x < 0  ||
		    Head().GetPosition().x >= M  )
		{
			return true;
		}

		for (int i = 0; i < size(); i++)
			for (int j = 0; j < size(); j++)
			{
				if (i == j) continue;
				if (at(i).GetPosition() == at(j).GetPosition())
					return true;
			}

		return false;
	}

	bool HasPosition(const Vector2i & pos) const
	{
		for (const auto & body : *this)
			if (body.GetPosition() == pos)
				return true;
		return false;
	}

	const Vector2i & GetPosition() const
	{
		return Head().GetPosition();
	}

	std::vector<Sprite> GetTiles() const
	{
		std::vector<Sprite> tiles;
		tiles.reserve(size());
		for (const auto & body : *this)
			tiles.push_back(body.GetTile());
		return tiles;
	}

	void Move(Direction dir)
	{
		// не даем ползти в текущую или противоположную сторону
		CheckDirection(dir);

		if (dir == Direction::NONE)
			dir = Head().GetDirection();

		// сохраняем хвост на случай, если его нужно будет отрастить
		tail_ = back();

		for (auto & body : *this)
		{
			dir = body.Move(dir);
			if (dir == Direction::NONE)
				dir = Head().GetDirection();
		}
	}

	void Eat()
	{
		push_back(tail_);
		++score_;
	}

	int GetScore() const
	{
		return score_;
	}

};

class Food : public GameObject
{
private:
	std::random_device rd_;

public:
	explicit Food(const Texture & texture): GameObject(texture)
	{
		tile_.setTextureRect(IntRect(STEP, 0, STEP, STEP));
	}

	void Reset()
	{
		std::mt19937 gen_ (rd_());

		std::uniform_int_distribution<> disX(0, M-1), disY(0, N-1);

		pos_.x = disX(gen_);
		pos_.y = disY(gen_);

		tile_.setPosition(static_cast<Vector2f>(pos_ * STEP));
	}

};

int main()
{
	RenderWindow window(sf::VideoMode(M*STEP, N*STEP), "Snake");

	enum SoundName
	{
		SN_MOVE, SN_EAT, SN_GAMEOVER, SN_TOTAL
	};

	SoundBuffer s_buffs[SN_TOTAL];
	s_buffs[SN_MOVE].loadFromFile("../data/move.ogg");
	s_buffs[SN_EAT].loadFromFile("../data/eat.ogg");
	s_buffs[SN_GAMEOVER].loadFromFile("../data/gameover.ogg");

	Sound sounds[SN_TOTAL];
	for (int i = SN_MOVE; i < SN_TOTAL; i++)
		sounds[i].setBuffer(s_buffs[i]);

	Texture texture;
	texture.loadFromFile("../data/tiles.png");

	Food food(texture);

	Snake snake = {
		Body(texture, Vector2i{2,0}),
		Body(texture, Vector2i{1,0}),
		Body(texture, Vector2i{0,0})
	};

	Clock clock;
	float timer = 0, delay = 0.5;
	bool GameOver = false, GameBegin = true;
	auto direction = Direction::NONE;

	auto reset_food = [&]()
	{
		do //сбрасываем так, чтобы небыло пересечений со змеей
		{
			food.Reset();
		}
		while (snake.HasPosition(food.GetPosition()));
	};

	while (window.isOpen())
	{
		timer += clock.getElapsedTime().asSeconds();
		clock.restart();

		Event event{};
		while (window.pollEvent(event))
		{
			if (event.type == Event::Closed)
				window.close();

			if (event.type == Event::KeyPressed && direction == Direction::NONE)
			{
				switch (event.key.code)
				{
					case Keyboard::Left:
						direction = Direction::LEFT; break;
					case Keyboard::Right:
						direction = Direction::RIGHT; break;
					case Keyboard::Up:
						direction = Direction::UP; break;
					case Keyboard::Down:
						direction = Direction::DOWN; break;
					default: ;
				}
			}
		}

		if (timer >= delay && !GameOver)
		{
			// проверяем пересечение со стенами и с собой
			GameOver = snake.CheckIntersects();
			if (GameOver)
			{
				sounds[SN_GAMEOVER].play();
				continue;
			}

			// двигаем всю змею согласно направлению на поле
			snake.Move(direction);

			// съедаем еду
			if (snake.GetPosition() == food.GetPosition())
			{
				snake.Eat();
				delay -= 0.01; // усложняем задачу
				reset_food();
				sounds[SN_EAT].play();
			}
			else sounds[SN_MOVE].play();

			timer = 0;
			direction = Direction::NONE;
		}

		if (GameBegin)
		{
			GameBegin = false;
			snake.Move(Direction::RIGHT);
			reset_food();
		}

		window.clear();

		if (GameOver)
		{
			Font font;
			font.loadFromFile("../data/kenney_blocks.ttf");
			Text text("Game Over\nScore: "+std::to_string(snake.GetScore()), font, 25);
			auto textRect = text.getLocalBounds();
			text.setOrigin(textRect.left + textRect.width/2.0f, textRect.top  + textRect.height/2.0f);
			text.setPosition(window.getSize().x/2.0f,window.getSize().y/2.0f);
			text.setFillColor(Color::Red);
			window.draw(text);
		}
		else
		{
			window.draw(food.GetTile());

			for (auto & tile : snake.GetTiles())
				window.draw(tile);
		}

		window.display();

	}

	return 0;
}