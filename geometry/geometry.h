#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <initializer_list>
#include <iostream>
#include <typeinfo>
#include <vector>

struct Point {
  double x = 0;
  double y = 0;
  static constexpr double eps = 0.0000001;

  Point(double x, double y) : x(x), y(y) {}

  Point() : x(0), y(0) {}

  bool operator==(Point other) const {
    return std::abs(x - other.x) < eps && std::abs(y - other.y) < eps;
  }

  double distance(const Point other) const {
    return sqrt(pow(x - other.x, 2) + pow(y - other.y, 2));
  }

  void rotateOnePoint(const Point& center, double angle) {
    double sin = std::sin(angle * M_PI / 180);
    double cos = std::cos(angle * M_PI / 180);
    *this = Point((x - center.x) * cos - (y - center.y) * sin + center.x,
                  (x - center.x) * sin + (y - center.y) * cos + center.y);
  }

  void reflectOnePointFromPoint(const Point& center) {
    x = 2 * center.x - x;
    y = 2 * center.y - y;
  }

  void scale(const Point& center, double coefficient) {
    x = center.x + (x - center.x) * coefficient;
    y = center.y + (y - center.y) * coefficient;
  }
};

class Line {
 private:
  static constexpr double eps = 0.00001;

 public:
  double A = 0;
  double B = 0;
  double C = 0;

  Line() = default;

  Line(const Point& first, const Point& second) {
    A = first.y - second.y;
    B = -first.x + second.x;
    C = -A * first.x - B * first.y;
  }

  Line(double angle, double shift) {
    A = -angle;
    B = 1;
    C = -shift;
  }

  Line(const Point& point, double angle)
      : Line(angle, point.y - angle * point.x) {}

  bool operator==(const Line& other) const {
    return std::abs(A * other.B - B * other.A) < eps &&
           std::abs(B * other.C - C * other.B) < eps;
  }

  bool operator!=(const Line& other) const { return !(*this == other); }

  double value(const Point& point) const {
    return A * point.x + B * point.y + C;
  }

  Line perpendicularLineFromPoint(const Point& point) const {
    Line answer;
    answer.A = B;
    answer.B = -A;
    answer.C = -answer.value(point);
    return answer;
  }

  Point intersection(const Line& other) const {
    double det = A * other.B - B * other.A;
    double a11 = other.B / det;
    double a12 = -B / det;
    double a21 = -other.A / det;
    double a22 = A / det;
    Point answer;
    answer.x = -C * a11 - other.C * a12;
    answer.y = -C * a21 - other.C * a22;
    return answer;
  }

  void reflectOnePointFromLine(Point& point) const {
    Line per_line = perpendicularLineFromPoint(point);
    Point H = intersection(per_line);
    point.reflectOnePointFromPoint(H);
  }
};

class Shape {
 public:
  virtual double perimeter() const = 0;

  virtual double area() const = 0;

  virtual bool operator==(const Shape& other) const = 0;

  virtual bool isCongruentTo(const Shape& other) const = 0;

  virtual bool isSimilarTo(const Shape& another) const = 0;

  virtual bool containsPoint(const Point& point) const = 0;

  virtual void rotate(const Point& center, double angle) = 0;

  virtual void reflect(const Point& center) = 0;

  virtual void reflect(const Line& axis) = 0;

  virtual void scale(const Point& center, double coefficient) = 0;

  virtual ~Shape() = default;
};

class Polygon : public Shape {
 private:
  void reversePoints() {
    size_t size = verticesCount();
    for (size_t i = 0; i < size / 2; ++i) {
      std::swap(points[i], points[size - i - 1]);
    }
  }

  double scalarProduct(const Point& first, const Point& second,
                       const Point& third, const Point& fourth) const {
    return (second.x - first.x) * (fourth.x - third.x) +
           (second.y - first.y) * (fourth.y - third.y);
  }

  double vectorProduct(const Point& first, const Point& second,
                       const Point& third, const Point& fourth) const {
    return (second.x - first.x) * (fourth.y - third.y) -
           (second.y - first.y) * (fourth.x - third.x);
  }

  bool isCongruentToWithoutSymmety(const Polygon* ptr) const {
    int size = static_cast<int>(verticesCount());
    for (int i = 0; i < size; ++i) {
      bool is_congruent = true;
      for (int j = 1; j < size && is_congruent; ++j) {
        if (std::abs(points[j - 1].distance(points[j]) -
                     ptr->points[(j + i - 1) % size].distance(
                         ptr->points[(j + i) % size])) > eps) {
          is_congruent = false;
        }
        double scalar_product = scalarProduct(
            points[j], points[j - 1], points[j], points[(j + 1) % size]);
        double other_scalar_product = scalarProduct(
            ptr->points[(j + i) % size], ptr->points[(j + i - 1) % size],
            ptr->points[(j + i) % size], ptr->points[(j + i + 1) % size]);
        if (std::abs(scalar_product *
                         ptr->points[(j + i) % size].distance(
                             ptr->points[(j + i - 1) % size]) *
                         ptr->points[(j + i) % size].distance(
                             ptr->points[(j + i + 1) % size]) -
                     other_scalar_product * points[j].distance(points[j - 1]) *
                         points[j].distance(points[(j + 1) % size])) > eps) {
          is_congruent = false;
        }
      }
      if (is_congruent) {
        return true;
      }
    }
    return false;
  }

  bool isSimilarToWithoutSymmetry(const Polygon* ptr) const {
    int size = static_cast<int>(verticesCount());
    double side = points[0].distance(points[1]);
    for (int i = 0; i < size; ++i) {
      bool is_similar = true;
      double other_side = ptr->points[i].distance(ptr->points[(i + 1) % size]);
      for (int j = 1; j < size && is_similar; ++j) {
        if (std::abs(points[j - 1].distance(points[j]) * other_side -
                     ptr->points[(j + i - 1) % size].distance(
                         ptr->points[(j + i) % size]) *
                         side) > eps) {
          is_similar = false;
        }
        double scalar_product = scalarProduct(
            points[j], points[j - 1], points[j], points[(j + 1) % size]);
        double other_scalar_product = scalarProduct(
            ptr->points[(j + i) % size], ptr->points[(j + i - 1) % size],
            ptr->points[(j + i) % size], ptr->points[(j + i + 1) % size]);
        if (std::abs(scalar_product *
                         ptr->points[(j + i) % size].distance(
                             ptr->points[(j + i - 1) % size]) *
                         ptr->points[(j + i) % size].distance(
                             ptr->points[(j + i + 1) % size]) -
                     other_scalar_product * points[j].distance(points[j - 1]) *
                         points[j].distance(points[(j + 1) % size])) > eps) {
          is_similar = false;
        }
      }
      if (is_similar) {
        return true;
      }
    }
    return false;
  }

 protected:
  std::vector<Point> points;
  static constexpr double eps = 0.01;

  enum Sign { minus = -1, zero = 0, plus = 1 };

  Sign sign(double x, double other_e) const {
    if (std::abs(x) < other_e) {
      return zero;
    }
    if (x > 0) {
      return plus;
    }
    return minus;
  }

 public:
  Polygon(const std::vector<Point>& points) : points(points) {}

  template <typename... Types>
  Polygon(const Types&... type) {
    (points.push_back(type), ...);
  }

  size_t verticesCount() const { return points.size(); }

  std::vector<Point> getVertices() const { return points; }

  bool isConvex() const {
    size_t size = points.size();
    if (size <= 3) {
      return true;
    }
    Sign convex = sign(
        (points[size - 1].x - points[0].x) * (points[1].y - points[0].y) -
            (points[1].x - points[0].x) * (points[size - 1].y - points[0].y),
        0);
    if (convex == zero) {
      return false;
    }
    for (size_t i = 0; i < size - 1; ++i) {
      if (convex != sign(vectorProduct(points[i + 1], points[i], points[i + 1],
                                       points[i + 2]),
                         0)) {
        return false;
      }
    }
    return true;
  }

  double perimeter() const override {
    if (points.empty()) {
      return 0;
    }
    double answer = points[points.size() - 1].distance(points[0]);
    for (size_t i = 0; i < points.size() - 1; ++i) {
      answer += points[i].distance(points[i + 1]);
    }
    return answer;
  }

  double area() const override {
    if (points.empty()) {
      return 0;
    }
    size_t size = verticesCount();
    double answer =
        points[size - 1].x * points[0].y - points[size - 1].y * points[0].x;
    for (size_t i = 0; i < size - 1; ++i) {
      answer += points[i].x * points[i + 1].y - points[i].y * points[i + 1].x;
    }
    return std::abs(answer / 2);
  }

  bool operator==(const Shape& other) const override {
    const Polygon* ptr = dynamic_cast<const Polygon*>(&other);
    if (ptr == nullptr) {
      return false;
    }
    int size = static_cast<int>(verticesCount());
    if (size != static_cast<int>(ptr->verticesCount())) {
      return false;
    }
    for (int i = -size + 1; i < size; ++i) {
      bool is_equal = true;
      for (int j = 0; j < size && is_equal; ++j) {
        if (points[j] != ptr->points[(i + j) % size]) {
          is_equal = false;
        }
      }
      if (is_equal) {
        return true;
      }
    }
    for (int i = -size + 1; i < size; ++i) {
      bool is_equal = true;
      for (int j = 0; j < size && is_equal; ++j) {
        if (points[j] != ptr->points[(size + i - j) % size]) {
          is_equal = false;
        }
      }
      if (is_equal) {
        return true;
      }
    }
    return false;
  }

  bool operator==(const Polygon& other) const {
    const Shape* other_copy = &other;
    return *this == *other_copy;
  }

  bool isCongruentTo(const Shape& other) const override {
    const Polygon* ptr = dynamic_cast<const Polygon*>(&other);
    if (ptr == nullptr) {
      return false;
    }
    int size = static_cast<int>(verticesCount());
    if (size != static_cast<int>(ptr->verticesCount())) {
      return false;
    }
    if (size <= 1) {
      return true;
    }
    bool first_compare = isCongruentToWithoutSymmety(ptr);
    if (first_compare) {
      return true;
    }
    Polygon reverse_poly = *ptr;
    reverse_poly.reversePoints();
    Polygon* reverse_ptr = &reverse_poly;
    bool second_compare = isCongruentToWithoutSymmety(reverse_ptr);
    return second_compare;
  }

  bool isSimilarTo(const Shape& other) const override {
    const Polygon* ptr = dynamic_cast<const Polygon*>(&other);
    if (ptr == nullptr) {
      return false;
    }
    int size = static_cast<int>(verticesCount());
    if (size != static_cast<int>(ptr->verticesCount())) {
      return false;
    }
    if (size <= 2) {
      return true;
    }
    bool first_compare = isSimilarToWithoutSymmetry(ptr);
    if (first_compare) {
      return true;
    }
    Polygon reverse_poly = *ptr;
    reverse_poly.reversePoints();
    Polygon* reverse_ptr = &reverse_poly;
    bool second_compare = isSimilarToWithoutSymmetry(reverse_ptr);
    return second_compare;
  }

  bool containsPoint(const Point& point) const override {
    size_t size = verticesCount();
    if (Line(point, points[0]) == Line(point, points[size - 1])) {
      return true;
    }
    for (size_t i = 0; i < size - 1; ++i) {
      if (Line(point, points[i]) == Line(point, points[i + 1])) {
        return true;
      }
    }
    double sin = vectorProduct(point, points[size - 1], point, points[0]);
    double cos = scalarProduct(point, points[size - 1], point, points[0]);
    double sum_of_angles = atan2(sin, cos);
    for (size_t i = 0; i < size - 1; ++i) {
      sin = vectorProduct(point, points[i], point, points[i + 1]);
      cos = scalarProduct(point, points[i], point, points[i + 1]);
      sum_of_angles += atan2(sin, cos);
    }
    return std::abs(sum_of_angles) > eps;
  }

  void rotate(const Point& center, double angle) override {
    size_t size = points.size();
    for (size_t i = 0; i < size; ++i) {
      points[i].rotateOnePoint(center, angle);
    }
  }

  void reflect(const Point& center) override {
    size_t size = points.size();
    for (size_t i = 0; i < size; ++i) {
      points[i].reflectOnePointFromPoint(center);
    }
  }

  void reflect(const Line& axis) override {
    size_t size = points.size();
    for (size_t i = 0; i < size; ++i) {
      axis.reflectOnePointFromLine(points[i]);
    }
  }

  void scale(const Point& center, double coefficient) override {
    size_t size = points.size();
    for (size_t i = 0; i < size; ++i) {
      points[i].scale(center, coefficient);
    }
  }

  virtual ~Polygon() override = default;
};

class Ellipse : public Shape {
 protected:
  Point focus1;
  Point focus2;
  double a;
  static constexpr double eps = 0.0000001;

 public:
  Ellipse(const Point& focus1, const Point& focus2, double a)
      : focus1(focus1), focus2(focus2), a(a / 2) {}

  std::pair<Point, Point> focuses() const { return {focus1, focus2}; }

  std::pair<Line, Line> directrices() const {
    double c = focus1.distance(focus2) / 2;
    Line line = Line(focus1, focus2);
    Line dir1 = line.perpendicularLineFromPoint(
        Point(focus1.x + (focus1.x - focus2.x) * a * a / (2 * c * c),
              focus1.y + (focus1.y - focus2.y) * a * a / (2 * c * c)));
    Line dir2 = line.perpendicularLineFromPoint(
        Point(focus2.x + (focus2.x - focus1.x) * a * a / (2 * c * c),
              focus2.y + (focus2.y - focus1.y) * a * a / (2 * c * c)));
    return {dir1, dir2};
  }

  double eccentricity() const { return focus1.distance(focus2) / (2 * a); }

  Point center() const {
    return Point((focus1.x + focus2.x) / 2, (focus1.y + focus2.y) / 2);
  }

  double perimeter() const override {
    double b = sqrt(a * a - pow(focus1.distance(focus2) / 2, 2));
    return M_PI * (3 * (a + b) - sqrt((3 * a + b) * (a + 3 * b)));
  }

  double area() const override {
    double b = sqrt(a * a - pow(focus1.distance(focus2) / 2, 2));
    return M_PI * a * b;
  }

  bool operator==(const Shape& other) const override {
    const Ellipse* ptr = dynamic_cast<const Ellipse*>(&other);
    if (ptr == nullptr) {
      return false;
    }
    if (a != ptr->a) {
      return false;
    }
    if ((focus1 == ptr->focus1 && focus2 == ptr->focus2) ||
        (focus1 == ptr->focus2 && focus2 == ptr->focus1)) {
      return true;
    }
    return false;
  }

  bool operator==(const Ellipse& other) const {
    const Shape* other_copy = &other;
    return *this == *other_copy;
  }

  bool isCongruentTo(const Shape& other) const override {
    const Ellipse* ptr = dynamic_cast<const Ellipse*>(&other);
    if (ptr == nullptr) {
      return false;
    }
    if (std::abs(focus1.distance(focus2) - ptr->focus1.distance(ptr->focus2)) <
            eps &&
        std::abs(a - ptr->a) < eps) {
      return true;
    }
    return false;
  }

  bool isSimilarTo(const Shape& other) const override {
    const Ellipse* ptr = dynamic_cast<const Ellipse*>(&other);
    if (ptr == nullptr) {
      return false;
    }
    if (std::abs(focus1.distance(focus2) * ptr->a -
                 ptr->focus1.distance(ptr->focus2) * a) < eps) {
      return true;
    }
    return false;
  }

  bool containsPoint(const Point& point) const override {
    return 2 * a - point.distance(focus1) - point.distance(focus2) > -eps;
  }

  void rotate(const Point& center, double angle) override {
    focus1.rotateOnePoint(center, angle);
    focus2.rotateOnePoint(center, angle);
  }

  void reflect(const Point& center) override {
    focus1.reflectOnePointFromPoint(center);
    focus2.reflectOnePointFromPoint(center);
  }

  void reflect(const Line& axis) override {
    axis.reflectOnePointFromLine(focus1);
    axis.reflectOnePointFromLine(focus2);
  }

  void scale(const Point& center, double coefficient) override {
    focus1.scale(center, coefficient);
    focus2.scale(center, coefficient);
    a *= std::abs(coefficient);
  }

  virtual ~Ellipse() override = default;
};

class Circle : public Ellipse {
 public:
  Circle(const Point& center, double r) : Ellipse(center, center, 2 * r) {}

  double radius() const { return a; }

  virtual ~Circle() override = default;
};

class Rectangle : public Polygon {
 public:
  Rectangle(const Point& first, const Point& second, double ratio) {
    if (ratio < 1) {
      ratio = 1 / ratio;
    }
    double catheter_near_second =
        first.distance(second) / sqrt(ratio * ratio + 1);
    double angle =
        std::atan2(catheter_near_second * ratio, catheter_near_second) * 180 /
        M_PI;
    Point point2(first.x + (second.x - first.x) * catheter_near_second /
                               first.distance(second),
                 first.y + (second.y - first.y) * catheter_near_second /
                               first.distance(second));
    point2.rotateOnePoint(first, angle);
    Point point4(first.x + second.x - point2.x, first.y + second.y - point2.y);
    points = {first, point2, second, point4};
  }

  Point center() const {
    return Point((points[0].x + points[1].x + points[2].x + points[3].x) / 4,
                 (points[0].y + points[1].y + points[2].y + points[3].y) / 4);
  }

  std::pair<Line, Line> diagonals() {
    return {Line(points[0], points[2]), Line(points[1], points[3])};
  }

  virtual ~Rectangle() override = default;
};

class Square : public Rectangle {
 public:
  Square(const Point& first, const Point& second)
      : Rectangle(first, second, 1) {}

  Circle circumscribedCircle() const {
    return Circle(center(), center().distance(points[0]));
  }

  Circle inscribedCircle() const {
    return Circle(center(), center().distance(points[0]) / sqrt(2));
  }

  virtual ~Square() override = default;
};

class Triangle : public Polygon {
 private:
  Point circumCenter() const {
    Point middle1((points[0].x + points[1].x) / 2,
                  (points[0].y + points[1].y) / 2);
    Point middle2((points[2].x + points[1].x) / 2,
                  (points[2].y + points[1].y) / 2);
    Line line1(points[0], points[1]);
    Line line2(points[2], points[1]);
    Line mid_al1 = line1.perpendicularLineFromPoint(middle1);
    Line mid_al2 = line2.perpendicularLineFromPoint(middle2);
    return mid_al1.intersection(mid_al2);
  }

  Point Incenter() const {
    double a = points[1].distance((points[2]));
    double b = points[0].distance((points[2]));
    double c = points[0].distance((points[1]));
    Point bis_base0(points[1].x + (points[2].x - points[1].x) * c / (b + c),
                    points[1].y + (points[2].y - points[1].y) * c / (b + c));
    Point bis_base1(points[0].x + (points[2].x - points[0].x) * c / (a + c),
                    points[0].y + (points[2].y - points[0].y) * c / (a + c));
    return Line(points[0], bis_base0).intersection(Line(points[1], bis_base1));
  }

 public:
  Triangle(const Point& first, const Point& second, const Point& third)
      : Polygon(first, second, third) {}

  Circle circumscribedCircle() const {
    Point circum_center = circumCenter();
    double radius = circum_center.distance(points[0]);
    return Circle(circum_center, radius);
  }

  Circle inscribedCircle() const {
    Point I = Incenter();
    Line line0(points[1], points[2]);
    Line per0 = line0.perpendicularLineFromPoint(I);
    Point tangent_point = line0.intersection(per0);
    return Circle(I, I.distance(tangent_point));
  }

  Point centroid() const {
    return Point((points[0].x + points[1].x + points[2].x) / 3,
                 (points[0].y + points[1].y + points[2].y) / 3);
  }

  Point orthocenter() const {
    Line line2(points[0], points[1]);
    Line line0(points[2], points[1]);
    Line altitude2 = line2.perpendicularLineFromPoint(points[2]);
    Line altitude0 = line0.perpendicularLineFromPoint(points[0]);
    return altitude0.intersection(altitude2);
  }

  Line EulerLine() const { return Line(centroid(), orthocenter()); }

  Circle ninePointsCircle() const {
    Triangle middle_triangle(
        Point((points[0].x + points[1].x) / 2, (points[0].y + points[1].y) / 2),
        Point((points[0].x + points[2].x) / 2, (points[0].y + points[2].y) / 2),
        Point((points[2].x + points[1].x) / 2,
              (points[2].y + points[1].y) / 2));
    return middle_triangle.circumscribedCircle();
  }

  virtual ~Triangle() override = default;
};

