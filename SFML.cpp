#include <SFML/Graphics.hpp>
#include <windows.h>
#include <iostream>
#include <vector>
#include <chrono>	
#include <map>
#include <random>
#include <algorithm>

using namespace std;

//Размер поля в пикселях
const int X_SIZE = 1900;
const int Y_SIZE = 1020;

//Размер одной клетки в пикселях
const int CELL_SIZE = 1;

//Продолжительность одного тика в миллисекундах
const int TIME_SPEED = 1;

//Размер поля в клетках
const int Xcells = X_SIZE / CELL_SIZE;
const int Ycells = Y_SIZE / CELL_SIZE;

//Двумерный массив содержащий освещенность поля
double field[Xcells][Ycells];
//Двумерный массив содержащий расположение клеток
int bacteriasPositions[Xcells][Ycells];

//Количество тиков с запуска программы
int tick = 0;

//ГПСЧ
mt19937 generator;
random_device device;

//Окно для изображения
sf::RenderWindow window(sf::VideoMode(X_SIZE, Y_SIZE), "SFML");

//Цвета для клеток
sf::Color colors[] = { sf::Color(130, 130, 130), sf::Color(0, 127, 127), sf::Color(0, 0, 255), sf::Color(255, 0, 0)};

//Класс содержащий внешний вид клетки
class Cell {
protected:
    sf::RectangleShape rectangle_;
    //[0] - тип поведения, [1] - количество энергии для размножения, [2] - максимальный возраст 
    double genome_[4];
    double energy_;
    int age_;
    int generation_;
    bool isReproduction_ = false;
    bool isDead_ = false;
public:
    Cell() { }

    void createRectangle(sf::Color fillColor, int x, int y) {
        rectangle_.setFillColor(fillColor);
        rectangle_.setSize(sf::Vector2f(CELL_SIZE, CELL_SIZE));
        rectangle_.setPosition(sf::Vector2f((float)x, (float)y));
    }

    sf::RectangleShape getRectangle() { return rectangle_; }
    void setRectangle(sf::RectangleShape rectangle) { rectangle_ = rectangle; }

    double* getGenome() { return genome_; }
    double getGenome(int index) { return genome_[index]; }
    void setGenome(double* genome) { for (int i = 0; i < 4; i++) { genome_[i] = genome[i]; } }
    void setGenome(int index, int value) { genome_[index] = value; }

    double getEnergy() { return energy_; }
    void setEnergy(double energy = 0) { energy_ = energy; }

    int getAge() { return age_; }
    void setAge(int age) { age_ = age; }

    int getGeneration() { return generation_; }
    void setGeneration(int generation) { generation_ = generation; }

    bool getReproduction() { return isReproduction_; }
    void setReproduction(bool isReproduction) { isReproduction_ = isReproduction; }

    bool getDead() { return isDead_; }

    int getX() { return getRectangle().getPosition().x / CELL_SIZE; }
    void setX(int x) {
        sf::RectangleShape rectangle = getRectangle();
        sf::Vector2f position = getRectangle().getPosition();
        position.x = (float)x;
        rectangle.setPosition(position);
        setRectangle(rectangle);
    }
    int getY() { return getRectangle().getPosition().y / CELL_SIZE; }
    void setY(int y) {
        sf::RectangleShape rectangle = getRectangle();
        sf::Vector2f position = getRectangle().getPosition();
        position.y = (float)y;
        rectangle.setPosition(position);
        setRectangle(rectangle);
    }

    //Мутирование генома
    void mutateGenome() {
        uniform_real_distribution<double> chance_of_mutation(0.0, 2.0);
        double chance = chance_of_mutation(generator);
        if (chance > 1.9) {
            //Выбираем какой ген мутирует
            uniform_int_distribution<int> genome_mutation(1, 3);
            int i = genome_mutation(generator);
            //Выбираем на сколько изменится
            uniform_real_distribution<> value_of_mutation(-0.1, 0.1);
            double value = value_of_mutation(generator);
            //Меняем значение гена
            genome_[i] = genome_[i] + genome_[i] * value;
            genome_[0] = 2;
            //Меняем цвет клетки
            sf::RectangleShape rect = getRectangle();
            sf::Color c(getRectangle().getFillColor());
            if(i == 1){ c.g -= value * 250; }
            else { c.b += value * 250; }
            rect.setFillColor(c);
            setRectangle(rect);
        }
    }
    virtual void step() = 0;
    virtual Cell* reproduction() = 0;
    virtual ~Cell() {}
};

//Струткура сравнения двух клеток по расстоянию для map
struct VectorCompare {
    bool operator() (const sf::Vector2f& l, const sf::Vector2f& r) const
    {
        return (l.x * l.x + l.y * l.y) < (r.x * r.x + r.y * r.y);
    }
};

map<sf::Vector2f, Cell*, VectorCompare> m;

//bool isNear(Cell* c) {
//    int x = abs(getX() - c->getX());
//    int y = abs(getY() - c->getY());
//    return sqrt(pow(x, 2) + pow(y, 2)) <= sqrt(2);
//}

//Класс бактерии
class PhotosynthesisBacteria : public Cell {
public:
    PhotosynthesisBacteria(double* genome = nullptr, int generation = 0, double energy = 0) {
        if (genome != nullptr) for (int i = 0; i < 4; i++) { genome_[i] = genome[i]; }
        else for (int i = 0; i < 4; i++) { genome_[i] = 0; }
        energy_ = energy;
        generation_ = generation;
        age_ = 0;
    }
    ~PhotosynthesisBacteria() {
        
    }



    //Выполнение хода клеткой
    void step() override {
        //Получение энергии
        energy_ = energy_ + field[getX()][getY()] - 45;

        //Поиск соседей
        int surroundCount = 0;
        for (int n = -1; n < 2; n++)
            for (int k = -1; k < 2; k++)
                if ((k != 0 || n != 0) && bacteriasPositions[(getX()) + n][(getY()) + k] == 1)
                    surroundCount++;

        //Решение, будет ли клтека размножаться
        if (energy_ > genome_[1] && surroundCount <= 2)
            isReproduction_ = true;
        else isReproduction_ = false;
        
        //Определение максимальной плотности колонии клеток
        if (surroundCount >= 3) {
            energy_ = energy_ - 1.1 * field[getX()][getY()];
        }
        
        //Определение смерти клетки
        if (energy_ < 0 || age_ > genome_[2]) {
            isDead_ = true;
        }

        
        if (m.size() > 1) {
            map<sf::Vector2f, Cell*, VectorCompare>::iterator nearestTarget = m.upper_bound(sf::Vector2f(rectangle_.getPosition().x, rectangle_.getPosition().y));
            if (nearestTarget == m.end()) {
                nearestTarget = m.lower_bound(sf::Vector2f(rectangle_.getPosition().x, rectangle_.getPosition().y));
            }
            //cout << nearestTarget->second->getX() << ", " << nearestTarget->second->getY() << endl;
        }
        
        
        age_++;
    }

    //Создание новой клетки
    Cell* reproduction() override {
        int direction[2] = { 1, 1 };
        bool isOccupuied = true;
        int cnt = 0;
        int x, y;

        for (int i = 0; i < 2; i++) {
            uniform_int_distribution<int> chance_direction(0, 100);
            int chance = chance_direction(generator);
            if (chance <= 33) direction[i] = -1;
            if (chance > 33 && chance <= 66) direction[i] = 0;
        }
        x = (getX()) + direction[0];
        if (x < 0) { x = (X_SIZE - CELL_SIZE) / CELL_SIZE; }
        else if (x >= X_SIZE / CELL_SIZE) x = 0;

        y = (getY()) + direction[1];

        if (0 <= y && y < Y_SIZE / CELL_SIZE && bacteriasPositions[x][y] == 0) {
            isOccupuied = false;
        }

        Cell* daugtherBacteria;
        if (x < 0) x = X_SIZE - CELL_SIZE;
        if (x > X_SIZE) x = 0;
        if (!isOccupuied) {
            daugtherBacteria = new PhotosynthesisBacteria(genome_, generation_ + 1, (energy_ / 2) - 100);
            daugtherBacteria->createRectangle(getRectangle().getFillColor(), x * CELL_SIZE, y * CELL_SIZE);
            daugtherBacteria->mutateGenome();
            energy_ -= ((energy_ / 2) - 100);
            bacteriasPositions[daugtherBacteria->getX()][daugtherBacteria->getY()] = 1;
            return daugtherBacteria;
        }
        else return nullptr;
    }
};

class PredatoryBacteria : public Cell {
public:
    PredatoryBacteria() {
        setGenome(0, 2);
    }
    ~PredatoryBacteria() {

    }

    void step() override {
        if (m.size() > 1) {
            //Ищет вокруг себя фотосинтезирующие клетки
            map<sf::Vector2f, Cell*, VectorCompare>::iterator nearestTarget = m.upper_bound(sf::Vector2f(rectangle_.getPosition().x, rectangle_.getPosition().y));
            if (nearestTarget == m.end()) {
                nearestTarget = m.lower_bound(sf::Vector2f(rectangle_.getPosition().x, rectangle_.getPosition().y));
            }

            nearestTarget = next(nearestTarget, 1);

            cout << "lower: " << nearestTarget->second->getX() << ", " << nearestTarget->second->getY() << endl;
            cout << "upper: " << nearestTarget->second->getX() << ", " << nearestTarget->second->getY() << endl;
            cout << "current: " << getX() << ", " << getY() << endl << endl;


            //if (nearestTarget == m.end()) {
            //    nearestTarget = m.lower_bound(sf::Vector2f(rectangle_.getPosition().x, rectangle_.getPosition().y));
            //}
            //cout << "near: " << nearestTarget->second->getX() << ", " << nearestTarget->second->getY() << endl;

            //Делает шаг
            sf::Vector2f pos = getRectangle().getPosition();
            if (pos.x < nearestTarget->second->getX()) {
                pos.x += CELL_SIZE;
            }
            else if (pos.x > nearestTarget->second->getX()) {
                pos.x -= CELL_SIZE;
            }

            if (pos.y < nearestTarget->second->getY()) {
                pos.y += CELL_SIZE;
            }
            else if (pos.y > nearestTarget->second->getY()) {
                pos.y -= CELL_SIZE;
            }


            sf::RectangleShape rect = getRectangle();
            rect.setPosition(pos);
            setRectangle(rect);
        }
        age_++;
    }

    Cell* reproduction() override {
        return nullptr;
    }
};

//Генерация рандомного double
double rand_d(double min, double max) {
    double f = (double)rand() / RAND_MAX;
    return min + f * (max - min);
}

//Генерация поля
void generateField() {
    for (int x = 0; x < Xcells; x++) {
        srand(time(0)/ rand());
        for (unsigned int y = 0; y < Ycells; y++) {
            double illuminance = rand_d(Y_SIZE / CELL_SIZE - y, (Y_SIZE / CELL_SIZE - y) + 5) * CELL_SIZE * 0.4 * 1.5;
            field[x][y] = illuminance;
        }
    }
}


//---------------------------------------------MAIN---------------------------------------------
int main()
{   
    //----------------Подготовка к старту симуляции----------------
    
    //Установка сида для ГПСЧ
    generator.seed(device());

    //Генерация фона
    generateField();

    //Заполнение матрицы бактерий нулями
    for (int x = 0; x < Xcells; x++)
        for (int y = 0; y < Ycells; y++)
            bacteriasPositions[x][y] = 0;

    //Добавление бактерии в первый вектор
    PhotosynthesisBacteria* bacteria = new PhotosynthesisBacteria(new double[] {1, 1000, 300, 0}, 1, 1);
    bacteria->createRectangle(colors[1], 700, 100);
    bacteriasPositions[bacteria->getX()][bacteria->getY()] = 1;
    m.emplace(bacteria->getRectangle().getPosition(), bacteria);

    //PredatoryBacteria* pBacteria = new PredatoryBacteria();
    //pBacteria->createRectangle(colors[3], 600, 200);
    //m.emplace(pBacteria->getRectangle().getPosition(), pBacteria);

    //----------------Цикл симуляции и отрисовки графики----------------
    while (window.isOpen())
    {
        //Проверка на закрытиые окна и очистка перед новым кадром
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }
        window.clear();
        
        //Имитация тиков в симуляции  
        Sleep(TIME_SPEED);

        for (auto& i : m) {
            if (!i.second->getDead()) {
                i.second->step();
                if (i.second->getReproduction()) {
                    Cell* newBacteria = i.second->reproduction();
                    if (newBacteria != nullptr && newBacteria->getRectangle().getPosition() != sf::Vector2f(0, 0)) {
                        m.emplace(newBacteria->getRectangle().getPosition(), newBacteria);
                    }
                }
            }
            window.draw(i.second->getRectangle());
        }
        for (auto& i : m) {
            if (i.second->getDead()) {
                bacteriasPositions[i.second->getX()][i.second->getY()] = 0;
                delete i.second;
                m.erase(i.first);
            }
        }
        tick++;
        window.display();
    }
    return 0;
}