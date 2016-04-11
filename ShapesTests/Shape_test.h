#include "Shape.h"
#include "Asserts.h"
#include "Factory.h"

namespace Shape_test
{
	using namespace Patchwork;
	static void test_circle()
	{
		int passed_test = 0;
		int nb_of_test = 13;
		Circle c = Circle(Vec2(50.f, 50.f), 35.f, Color(255, 125, 0));
		passed_test += test_assert(c.origin() == Vec2(50.f, 50.f), "c.origin() == Vec2(50.f, 50.f)");
		passed_test += test_assert(c.radius() == 35.f, "c.radius() == 35.f");
		passed_test += test_assert(c.color() == Color(255, 125, 0), "c.color() == Color(255, 125,0)");

		Circle c2 = Circle(c);
		passed_test += test_assert(c == c2, "c == c2 ......");

		passed_test += test_assert(c.area() == (float)(PI*35.f*35.f), "c.area() == (PI*35.f*35.f)");
		passed_test += test_assert(c.perimeter() == (float)(2 * PI*35.f), "c.perimeter() == (2 * PI*35.f)");
		
		c.translate(Vec2(10.f, 10.f));
		passed_test += test_assert(c.origin() == Vec2(60.f, 60.f), "c.origin() == Vec2(60.f, 60.f)");
		c.centralSym(Vec2(0.f, 0.f));
		passed_test += test_assert(c.origin() == Vec2(-60.f, -60.f), "c.origin() == Vec2(-60.f, -60.f)") ;

		c2 = Circle(c);
		c2.centralSym(c2.origin());
		passed_test += test_assert(c == c2, "c == c2");
		c.homothety( 2.5f );
		passed_test += test_assert(c.radius() == (35.f * 2.5f), "c.radius() == (35.f * 2.5f)");
		c.axialSym(Vec2(0.f, 0.f), Vec2(0.f, 1.f));
		passed_test += test_assert(c.origin() == Vec2(60.f, -60.f), "c.origin() == Vec2(60.f, -60.f)");

		Circle c3 = Circle(Vec2(0.f, 1.f), 10, Color());
		c3.rotate(DEGTORAD * 90);
		passed_test += test_assert(c3.origin() == Vec2(0.f, 1.f), "c3.origin() == Vec2(0.f, 1.f)");
		std::string s;
		c3.serialize(s);
		passed_test += test_assert(s == " circle 0.00 1.00 10.00 0 0 0", "c3.serialize() == serial");

		std::cout << "Test class Circle : " << (int)(((float)passed_test / nb_of_test)*100) << "% OK !" << std::endl;
	}

	static void test_polygon()
	{
		int passed_test = 0;
		int nb_of_test = 6;
		Polygon p = Polygon({ { 0, 1 }, { 1, 1 }, { 1, 0 }, { 0, 0 } }, Color());
		passed_test += test_assert(p.area() == 1, "p.area() == 1");
		passed_test += test_assert(p.perimeter() == 4, "p.perimeter() == 4");
		Polygon p2 = Polygon(p);
		passed_test += test_assert(p == p2, "p == p2");
		p2.translate(Vec2(10.f, 15.f));
		passed_test += test_assert(p != p2, "p != p2");
		p2.translate(Vec2(-10.f, -15.f));
		passed_test += test_assert(p == p2, "p == p2");
		std::string s;
		p2.serialize(s);
		passed_test += test_assert(s == " polygon 4 0.00 1.00 1.00 1.00 1.00 0.00 0.00 0.00 0 0 0", "p2.serialize() == serial");

		std::cout << "Test class Polygon : " << (int)(((float)passed_test / nb_of_test) * 100) << "% OK !" << std::endl;
	}

	static void test_ellipse()
	{
		Ellipse e = Ellipse(Vec2(0, 0), Vec2(10, 3), Color());
		test_assert(e.perimeter() == 62.8318520f, "e.perimeter() == 62.8318520");
		std::cout << e.perimeter() << std::endl;
	}

	static void test_line()
	{
		std::cout << fast_cos(0) << std::endl;
	}

	static void run_tests()
	{
		test_circle();
		std::cout << std::endl;
		test_polygon();
		std::cout << std::endl;
		test_ellipse();
		std::cout << std::endl;
		test_line();
	}
}