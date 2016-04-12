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

		std::cout << "Begin test suit for Circle" << std::endl << std::endl;

		//test creation
		Circle c = Circle(Vec2(50.f, 50.f), 35.f, Color(255, 125, 0));
		passed_test += test_assert(c.origin() == Vec2(50.f, 50.f), "Creation : Origin");
		passed_test += test_assert(c.radius() == 35.f, "Creation : Radius");
		passed_test += test_assert(c.color() == Color(255, 125, 0), "Creation : Color");

		//test clone and equal
		Circle c2 = Circle(c);
		passed_test += test_assert(c == c2, "Clonage");

		//test area
		passed_test += test_assert(c.area() == (float)(PI*35.f*35.f), "Area");

		//test perimeter
		passed_test += test_assert(c.perimeter() == (float)(2 * PI*35.f), "Perimeter");
		
		//test transforms
		c.translate(Vec2(10.f, 10.f));
		passed_test += test_assert(c.origin() == Vec2(60.f, 60.f), "Translation");
		c.centralSym(Vec2(0.f, 0.f));
		passed_test += test_assert(c.origin() == Vec2(-60.f, -60.f), "Central sym") ;

		c2 = Circle(c);
		c2.centralSym(c2.origin());
		passed_test += test_assert(c == c2, "Central sym2");
		c.homothety( 2.5f );
		passed_test += test_assert(c.radius() == (35.f * 2.5f), "Homothety");
		c.axialSym(Vec2(0.f, 0.f), Vec2(0.f, 1.f));
		passed_test += test_assert(c.origin() == Vec2(60.f, -60.f), "Axial sym");

		Circle c3 = Circle(Vec2(0.f, 1.f), 10, Color());
		c3.rotate(DEGTORAD * 90);
		passed_test += test_assert(c3.origin() == Vec2(0.f, 1.f), "Rotate");
		
		//test serialize
		std::string s;
		c3.serialize(s);
		passed_test += test_assert(s == " circle 0.00 1.00 10.00 0 0 0", "Serialize");

		std::cout << std::endl << "Test class Circle : " << (int)(((float)passed_test / nb_of_test) * 100) << "% OK !" << std::endl;
	}

	static void test_polygon()
	{
		int passed_test = 0;
		int nb_of_test = 9;

		std::cout << "Begin test suit for Polygon" << std::endl << std::endl;

		Polygon p = Polygon({ { 0, 1 }, { 1, 1 }, { 1, 0 }, { 0, 0 } }, Color());
		passed_test += test_assert(p.area() == 1, "Area");
		passed_test += test_assert(p.perimeter() == 4, "Perimeter");
		Polygon p2 = Polygon(p);
		passed_test += test_assert(p == p2, "Clonage");
		p2.translate(Vec2(10.f, 15.f));
		passed_test += test_assert(p != p2, "Translation");
		p2.translate(Vec2(-10.f, -15.f));
		passed_test += test_assert(p == p2, "Translation 2");
		p.homothety(0.5);
		passed_test += test_assert((p.area() == 0.25f) && (p.perimeter() == 2), "Homothety");
		Polygon p3 = Polygon({ { 0, 1 }, { -1, 1 }, { -1, 0 }, { 0, 0 } }, Color());
		p2.axialSym(Vec2(0, 0), Vec2(0, 1));
		passed_test += test_assert(p2 == p3, "Axial sym");
		Polygon p4 = Polygon({ { 0, -1 }, { 1, -1 }, { 1, 0 }, { 0, 0 } }, Color());
		p2.centralSym(Vec2(0,0));
		passed_test += test_assert(p2 == p4, "Central sym");
		std::string s;
		p2.serialize(s);
		passed_test += test_assert(s == " polygon 4 0.00 -1.00 1.00 -1.00 1.00 0.00 0.00 0.00 0 0 0", "Serialize");

		std::cout << std::endl << "Test class Polygon : " << (int)(((float)passed_test / nb_of_test) * 100) << "% OK !" << std::endl;
	}

	static void test_ellipse()
	{
		int passed_test = 0;
		int nb_of_test = 3;

		std::cout << "Begin test suit for Ellipse" << std::endl << std::endl;

		Ellipse e = Ellipse(Vec2(0, 0), Vec2(10, 3), Color());
		passed_test += test_assert(  (e.perimeter() <= 43.8591f ) && (e.perimeter() >= 43.8590f), "Perimetre");
		passed_test += test_assert((e.area() <= 94.2478f) && (e.area() >= 94.2477f), "Aire");
		e.homothety( 0.5f );
		passed_test += test_assert(e.radius() == Vec2(5, 1.5f), "Homothety");
		//central sym, axial, sym, translate, etc, same as circle, don't need to test it again

		std::cout << std::endl << "Test class Ellipse  : " << (int)(((float)passed_test / nb_of_test) * 100) << "% OK !" << std::endl;
	}

	static void test_line()
	{
		int passed_test = 0;
		int nb_of_test = 2;

		std::cout << "Begin test suit for Line" << std::endl << std::endl;

		Line l = Line(Vec2(0, 0), Vec2(0, 1), Color());
		l.rotate(DEGTORAD * 90);
		passed_test += test_assert(l == Line(Vec2(0, 0), Vec2(-1, 0.f), Color()), "Rotation");
		l.rotate(Vec2(1, 0), DEGTORAD * 90);
		passed_test += test_assert(l == Line(Vec2(1, -1), Vec2(0, -1), Color()), "Rotation");

		std::cout << std::endl << "Test class Line  : " << (int)(((float)passed_test / nb_of_test) * 100) << "% OK !" << std::endl;
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