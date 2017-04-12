#ifndef STAR_H
#define STAR_H


class Star
{
public:
    Star();
    Star(int x_, int y_, float value_);

    ~Star();

    bool operator==(const Star& s);
    bool operator>(const Star& other) const;
    bool operator<(const Star& other) const;

    int getX() const;
    void setX(int value);

    int getY() const;
    void setY(int value);

    float getPeak() const;
    void setPeak(float value);

    float getRadius() const;
    void setRadius(float value);

    int getArea() const;
    void setArea(int value);

    float getValue() const;
    void setValue(float newValue);

private:
    int x;
    int y;
    int area;

    float peak;
    float radius;
    float value;
};

#endif // STAR_H
