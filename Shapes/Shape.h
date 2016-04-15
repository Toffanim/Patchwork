#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <mutex>
#include <algorithm>
#include "Maths.h"
#include "SDL2\SDL.h"

/*! \file Shape.h
\brief Header files containing all things related to geometric shapes.

Gives access to the classes : Circle, Polygon, Line and Ellipse, which all inherits from the Shape class.
Also provides the Image class which is a container of shapes.

Details.
*/

namespace Patchwork
{
	/*! 
	Function to transform a floating point number into a std::string
	This function is needed because we wants only two decimals printed from the float.
	*/
	std::string to_string(float f)
	{
		std::stringstream stream;
		stream << std::fixed << std::setprecision(2) << f;
		return stream.str();
	}

	/*!
	Function to transform a integer into a std::string
	This function is not particularly needed since std::to_string(int) exists.
	*/
	std::string to_string(int i)
	{
		std::stringstream stream;
		stream << i;
		return stream.str();
	}

	/*!
	A structure for a bounding box object defined by two points :
	upper left and lower right corners.
	The structure is initialized with minimal value for maximums and maximal value for minimums (integer 10000), so it can be used
	right away to compute min and max.
	*/
	struct BoundingBox
	{
		int x_max; /*!< Lower corner x coordinate */
		int x_min; /*!< Upper corner x coordinate */
		int y_max; /*!< Lower corner y coordinate */
		int y_min; /*!< Upper corner y coordinate*/
		BoundingBox() :x_max(-10000), y_max(-10000), x_min(10000), y_min(10000){}
	};

	class Transformable
	{
		virtual float area() = 0;
		virtual float perimeter() = 0;
		virtual void translate(const Vec2& t) = 0;
		virtual void homothety(float ratio) = 0;
		virtual void homothety(const Vec2& s, float ratio) = 0;
		virtual void rotate(float angle) = 0;
		virtual void rotate(const Vec2& p, double angle) = 0;
		virtual void centralSym(const Vec2& c) = 0;
		virtual void axialSym(const Vec2& p, const Vec2& d) = 0;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	/*!
	An abstract class which define a 2D Shape.
	All function supposed the space to be two dimensional.
	*/
	class Shape
	{
	public:
		enum Derivedtype { CIRCLE=0, POLYGON, LINE, ELLIPSE, IMAGE, END_ENUM }; /*!< Enum of available derived types */
		enum Functions { ROTATION = 0, HOMOTHETY, TRANSLATE, AXIAL_SYMETRY, CENTRAL_SYMETRY, UNKNOWN }; /*!< Enum of available transforamtions */
		static const std::vector<std::string> transforms; /*!< A static container of strings defining the transformation string assiciaited to its Functions enum value  */
		static const std::vector<std::string> shapes;  /*!< A static container of strings defining the shape string assiciated to its Derivedtype enum value  */

		/*!
		Static function to print available transfromation keywords
		*/
		static void print_transforms()
		{
			for (auto transform : transforms)
				std::cout << " " << transform;
		}
		/*!
		Static function to print available shapes keywords
		*/
		static void print_shapes()
		{
			for (auto shape : shapes)
				std::cout << " " << shape;
		}
		/*!
		Static function to convert a string into a Derivedtype enum.
		Return END_ENUM if not in the container
		*/
		static Derivedtype ShapeStringToEnum(std::string s)
		{
			for (int i = 0; i < shapes.size(); ++i)
			{
				if (shapes.at(i) == s)
				{
					return static_cast<Derivedtype>(i);
				}
			}
			return Derivedtype::END_ENUM;
		}
		/*!
		Static function to convert a string into a Functions enum.
		Return UNKNOWN if not in the container.
		*/
		static Functions FuncStringToEnum(std::string s)
		{
			for (int i = 0; i < transforms.size(); ++i)
			{
				if (transforms.at(i) == s)
				{
					return static_cast<Functions>(i);
				}
			}
			return Functions::UNKNOWN;
		}
		/*!
		Constructor initializing the Derivedtype and color
		*/
		Shape(Derivedtype type, Color color) : m_type(type), m_color(color){};
		~Shape(){};
		/*!
		Getter for the variable type
		*/
		Derivedtype type() const { return(m_type); }
		/*!
		Getter for the variable color
		*/
		const Color color() const { return(m_color); }
		/*!
		Interface function, needed in inheriting classes, to compute the area of the shape.
		*/
		virtual float area() = 0;
		/*!
		Interface function, needed in inheriting classes, to compute the perimeter of the shape.
		*/
		virtual float perimeter() = 0;
		/*!
		Interface function, needed in inheriting classes, to compute the translation of the shape by a vector v
		*/
		virtual void translate(const Vec2& v) = 0;
		/*!
		Interface function, needed in inheriting classes, to compute the homothety of the shape by a ratio.
		The ratio is supposed to be a float which value is comprised between [0, +infinity]. The origin of the homothety is the center of the shape' bounding box.
		*/
		virtual void homothety(float ratio) = 0;
		/*!
		Interface function, needed in inheriting classes, to compute the homothety of the shape by a ratio and an origin o.
		The ratio is supposed to be a float which value is comprised between [0, +infinity]. The origin of the homothety is the point o.
		*/
		virtual void homothety(const Vec2& o, float ratio) = 0;
		/*!
		Interface function, needed in inheriting classes, to compute the rotation of the shape by an angle in radiant.
		The origin of the rotation is the center of the shape' bounding box.
		*/
		virtual void rotate(float angle) = 0;
		/*!
		Interface function, needed in inheriting classes, to compute the rotation of the shape by an angle in radiant and an origin o.
		The origin of the rotation is the point o.
		*/
		virtual void rotate(const Vec2& o, double angle) = 0;
		/*!
		Interface function, needed in inheriting classes, to compute the central symetry of the shape with origin o.
		The origin of the symetry is the point o.
		*/
		virtual void centralSym(const Vec2& o) = 0;
		virtual void axialSym(const Vec2& p, const Vec2& d) = 0;
		virtual void display(SDL_Renderer* renderer, float ratio) = 0;
		virtual void serialize( std::string& serial ) = 0;
		virtual BoundingBox bounding_box() = 0;

		friend std::ostream& operator<< (std::ostream &out, Shape &Shape);
	protected:
		Derivedtype m_type;
		Color m_color;
	};
	const std::vector<std::string> Shape::transforms = { "rotate", "homothety", "translate", "axial_sym", "central_sym" };
	const std::vector<std::string> Shape::shapes = { "circle", "polygon", "line", "ellipse" };

	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	class Circle : public Shape
	{
	public:
		Circle(Vec2 origin, float radius, Color color) :
			Shape(Shape::Derivedtype::CIRCLE, color),
			m_origin(origin),
			m_radius(radius)
		{}


		//Circle specific functions
		const Vec2& origin() const { return (m_origin); }
		const float radius() const { return(m_radius); }

		//Transformable interface functions
		float area() { return(PI * (m_radius*m_radius)); }
		float perimeter() { return(2 * PI* m_radius); }
		void homothety(float ratio) { m_radius *= ratio; }
		void homothety(const Vec2& s, float ratio) 
		{  
			Vec2 u = ( m_origin - s);
			m_origin = s + ratio*u;
			m_radius *= ratio;
		}
		void rotate(const Vec2& p, double angle)
		{
			float s = fast_sin(angle);
			float c = fast_cos(angle);
			m_origin.x -= p.x;
			m_origin.y -= p.y;
			float x = (m_origin.x * c - m_origin.y * s) + p.x;
			float y = (m_origin.x * s + m_origin.y * c) + p.y;
			m_origin.x = x;
			m_origin.y = y;
		}
		void rotate(float angle)
		{
			//Use origin as rotation center
			//does not change anythiong
		}
		void translate(const Vec2& t) { m_origin = m_origin + t; }
		void centralSym(const Vec2& c) { Vec2 t = 2 * (c - m_origin); translate(t); }
		void axialSym(const Vec2& p, const Vec2& d)
		{
			Vec2 p1 = p + d;
			Vec2 w = m_origin - p;
			Vec2 vl = p1 - p;
			float b = dot(w, vl) / dot(vl, vl);
			Vec2 intersection = p + b * vl;
			translate(2 * (intersection - m_origin));
		}

		//Displayable interface functions
		void display(SDL_Renderer* renderer, float ratio)
		{
			if (ratio != 1.f)
			{
				Circle *c = new Circle( *this );
				c->homothety(Vec2(0, 0), ratio);
				c->display(renderer, 1.f);
				delete c;
			}
			else
			{
				SDL_SetRenderDrawColor(renderer, m_color.r, m_color.g, m_color.b, 0x00);
				int w, h;
				SDL_GetRendererOutputSize(renderer, &w, &h);
				Vec2 center((w / 2), (h / 2));
				Vec2 displayablePoint = m_origin + center;
				for (int i = -(int)(m_radius); i < (int)(m_radius); ++i)
				{
					for (int j = -(int)(m_radius); j < (int)(m_radius); ++j)
					{
						if (i*i + j*j <= m_radius*m_radius)
						{
							SDL_RenderDrawPoint(renderer, (int)(displayablePoint.x + i), (int)(displayablePoint.y + j));
						}
					}
				}
			}
		}

		void serialize(std::string& serial)
		{
			serial = serial + " circle";
			serial = serial + " " + to_string(m_origin.x);
			serial = serial + " " + to_string(m_origin.y);
			serial = serial + " " + to_string(m_radius);
			serial = serial + " " + to_string(m_color.r);
			serial = serial + " " + to_string(m_color.g);
			serial = serial + " " + to_string(m_color.b);
		}

		BoundingBox bounding_box()
		{
			BoundingBox bb = {};
			bb.x_min = (int)(m_origin.x - m_radius);
			bb.x_max = (int)(m_origin.x + m_radius);
			bb.y_min = (int)(m_origin.y - m_radius);
			bb.y_max = (int)(m_origin.y + m_radius);
			
			return bb;
		}


		friend std::ostream& operator<< (std::ostream &out, const Circle &Circle);
	private:
		Vec2 m_origin;
		float m_radius;
	};

	bool operator==(const Circle& a, const Circle& b) { if (a.origin() == b.origin() && a.radius() == b.radius() && a.color() == b.color()) return true; return false; }
	bool operator!=(const Circle& a, const Circle& b) { return !(a == b); }
	std::ostream& operator<< (std::ostream &out, const Circle &Circle)
	{
		out << "Circle" << std::endl;
		out << "\t" << Circle.origin() << " " << Circle.radius() << " " << Circle.color() << std::endl;
		return out;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	class Polygon : public Shape
	{
	public:
		Polygon(std::vector<Vec2> points, Color color) :
			Shape(Shape::Derivedtype::POLYGON, color),
			m_points(points)
		{
			//ASSERT VEC SIZE >= 3
		}

		//Polygon specific functions
		const std::vector<Vec2>& points() const { return(m_points); }

		//Transformable interface functions
		float area()
		{
			float a = 0.f;
			for (std::vector<Vec2>::const_iterator it = m_points.begin() + 1; it != m_points.end() - 1; ++it)
			{
				a += triangle_area(m_points[0], (*it), (*(it + 1)));
			}
			return(a);
		}

		float perimeter()
		{
			float p = 0.f;
			for (std::vector<Vec2>::const_iterator it = m_points.begin(); it != m_points.end(); ++it)
			{
				if (it == m_points.end() - 1)
					p += norm((*it) - (*(m_points.begin())));
				else
					p += norm((*it) - (*(it + 1)));
			}
			return(p);
		}

		void homothety(float ratio) 
		{ /*compute bounding rectangle and move points by (rect_center - points)*ratio */ 
			BoundingBox bb = bounding_box();
			Vec2 center = Vec2(bb.x_max - ((bb.x_max - bb.x_min) / 2.f), bb.y_max - ((bb.y_max - bb.y_min) / 2.f));
			for (std::vector<Vec2>::iterator it = m_points.begin(); it != m_points.end(); ++it)
			{
				Vec2 u = ((*it) - center);
				(*it) = center + ratio*u;
			}
		}
		void homothety(const Vec2& s, float ratio) 
		{  
			for (std::vector<Vec2>::iterator it = m_points.begin(); it != m_points.end(); ++it)
			{
				Vec2 u = ((*it) - s);
				(*it) = s + ratio*u;
			}
		}
		void rotate(const Vec2& p, double angle)
		{
			for (std::vector<Vec2>::iterator it = m_points.begin(); it != m_points.end(); ++it)
			{
				float s = fast_sin(angle);
				float c = fast_cos(angle);
				(*it).x -= p.x;
				(*it).y -= p.y;
				float x = ((*it).x * c - (*it).y * s) + p.x;
				float y = ((*it).x * s + (*it).y * c) + p.y;
				(*it).x = x;
				(*it).y = y;
			}
		}
		void rotate(float angle)
		{
			for (std::vector<Vec2>::iterator it = m_points.begin(); it != m_points.end(); ++it)
			{
				float s = fast_sin(angle);
				float c = fast_cos(angle);
				float x = ((*it).x * c - (*it).y * s);
				float y = ((*it).x * s + (*it).y * c);
				(*it).x = x;
				(*it).y = y;
			}
		}
		void translate(const Vec2& t)
		{
			for (std::vector<Vec2>::iterator it = m_points.begin(); it != m_points.end(); ++it)
			{
				(*it) = (*it) + t;
			}
		}
		void centralSym(const Vec2& c)
		{
			for (std::vector<Vec2>::iterator it = m_points.begin(); it != m_points.end(); ++it)
			{
				Vec2 t = 2 * (c - (*it));
				(*it) = (*it) + t;
			}
		}
		void axialSym(const Vec2& p, const Vec2& d)
		{
			for (std::vector<Vec2>::iterator it = m_points.begin(); it != m_points.end(); ++it)
			{
				Vec2 p1 = p + d;
				Vec2 w = (*it) - p;
				Vec2 vl = p1 - p;
				float b = dot(w, vl) / dot(vl, vl);
				Vec2 intersection = p + b * vl;
				(*it) = (*it) + (2 * (intersection - (*it)));
			}
		}

		void display(SDL_Renderer* renderer, float ratio)
		{
			if (ratio != 1.f)
			{
				Polygon *c = new Polygon(*this);
				c->homothety(Vec2(0, 0), ratio);
				c->display(renderer, 1.f);
				delete c;
			}
			else
			{
				SDL_SetRenderDrawColor(renderer, m_color.r, m_color.g, m_color.b, 0x00);
				int w, h;
				SDL_GetRendererOutputSize(renderer, &w, &h);
				Vec2 center((w / 2), (h / 2));
				BoundingBox bb = bounding_box();
				for (int i = bb.x_min - 1; i < bb.x_max + 1; ++i)
				{
					for (int j = bb.y_min - 1; j < bb.y_max + 1; ++j)
					{
						if (isPointInPolygon(Vec2(i, j)))
						{
							SDL_RenderDrawPoint(renderer, i + center.x, j + center.y);
						}
					}
				}
			}
		}

		void serialize(std::string& serial)
		{
			serial = serial + " polygon";
			serial = serial + " " + to_string((int)m_points.size());
			for (auto point : m_points)
			{
				serial = serial + " " + to_string(point.x);
				serial = serial + " " + to_string(point.y);
			}
			serial = serial + " " + to_string(m_color.r);
			serial = serial + " " + to_string(m_color.g);
			serial = serial + " " + to_string(m_color.b);
		}

		BoundingBox bounding_box()
		{
			BoundingBox bb;
			for (auto point : m_points)
			{
				if (point.x < bb.x_min)
					bb.x_min = (int)point.x;
				if (point.x > bb.x_max)
					bb.x_max = (int)point.x;
				if (point.y < bb.y_min)
					bb.y_min = (int)point.y;
				if (point.y > bb.y_max)
					bb.y_max = (int)point.y;					
			}
			return bb;
		}

		friend std::ostream& operator<< (std::ostream &out, const Polygon &Polygon);

	private:
		std::vector<Vec2> m_points;
		float triangle_area(Vec2 a, Vec2 b, Vec2 c) const { return((1.f / 2.f) * abs((b.x - a.x)*(c.y - a.y) - (c.x - a.x)*(b.y - a.y))); }
		bool isPointInPolygon(Vec2 p) {
			int i, j, nvert = m_points.size();
			bool c = false;

			for (i = 0, j = nvert - 1; i < nvert; j = i++) {
				if (((m_points[i].y >= p.y) != (m_points[j].y >= p.y)) &&
					(p.x <= (m_points[j].x - m_points[i].x) * (p.y - m_points[i].y) / (m_points[j].y - m_points[i].y) + m_points[i].x)
					)
					c = !c;
			}
			return c;
		}
	};

	bool operator==(const Polygon& a, const Polygon& b)
	{
		if (a.points() == b.points())
			return true;
		return false;
	}

	bool operator!=(const Polygon& a, const Polygon& b)
	{
		return !(a == b);
	}

	std::ostream& operator<< (std::ostream &out, const Polygon &Polygon)
	{
		out << "Polygon" << std::endl;
		for (auto point : Polygon.points())
		{
			out << "\t" << point << std::endl;
		}
		out << "\t" << Polygon.color() << std::endl;
		return out;
	}
	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	class Line : public Shape
	{
	public:
		Line(Vec2 point, Vec2 direction, Color color) :
			Shape(Shape::Derivedtype::LINE, color),
			m_point(point),
			m_direction(direction)
		{};

		//Line specific functions
		const Vec2& point() const { return (m_point); }
		const Vec2& direction() const { return (m_direction); }

		//Transformable interface functions
		float area() { return(1.f); }
		float perimeter() { return(1.f); }
		void homothety(float ratio) { /*NON SENSE*/ }
		void homothety(const Vec2& s, float ratio) { /*NON SENSE*/ }
		void rotate(float angle) 
		{  
			Vec2 p = m_point + m_direction;
			p.x -= m_point.x;
			p.y -= m_point.y;
			float s = fast_sin(angle);
			float c = fast_cos(angle);
			float x = (p.x * c - p.y * s) + m_point.x;
			float y = (p.x * s + p.y * c) + m_point.y;

			m_direction = (Vec2(x, y) - m_point);
		}
		void rotate(const Vec2& p, double angle) 
		{ 
			Vec2 point = m_point + m_direction;
			point.x -= p.x;
			point.y -= p.y;
			float s = fast_sin(angle);
			float c = fast_cos(angle);
			float x = (point.x * c - point.y * s) + p.x;
			float y = (point.x * s + point.y * c) + p.y;

			m_point.x -= p.x;
			m_point.y -= p.y;
			float c_x = (m_point.x * c - m_point.y * s) + p.x;
			float c_y = (m_point.x * s + m_point.y * c) + p.y;
			m_point.x = c_x;
			m_point.y = c_y;
			m_direction = (Vec2(x, y) - m_point);
		}

		void translate(const Vec2& t) { m_point = m_point + t; }
		void centralSym(const Vec2& c) { Vec2 t = 2 * (c - m_point); translate(t); }
		void axialSym(const Vec2& p, const Vec2& d){ /*NON SENSE*/ }
		void display(SDL_Renderer* renderer, float ratio)
		{
			SDL_SetRenderDrawColor(renderer, m_color.r, m_color.g, m_color.b, 0x00);
			int w, h;
			SDL_GetRendererOutputSize(renderer, &w, &h);
			Vec2 center((w / 2), (h / 2));
			Vec2 displayablePoint = m_point + center;
			SDL_RenderDrawLine(renderer, (int)displayablePoint.x, (int)displayablePoint.y, (int)(displayablePoint.x + m_direction.x), (int)(displayablePoint.y + m_direction.y));
		}

		void serialize(std::string& serial)
		{
			serial = serial + " line";
			serial = serial + " " + to_string(m_point.x);
			serial = serial + " " + to_string(m_point.y);
			serial = serial + " " + to_string(m_direction.x);
			serial = serial + " " + to_string(m_direction.y);
			serial = serial + " " + to_string(m_color.r);
			serial = serial + " " + to_string(m_color.g);
			serial = serial + " " + to_string(m_color.b);
		}

		BoundingBox bounding_box()
		{
			BoundingBox bb;
			Vec2 p2 = m_point + m_direction;
			if (p2.x < m_point.x)
			{	bb.x_min = p2.x;
			    bb.x_max = m_point.x;
		    }
			if (p2.y < m_point.y)
			{
				bb.y_min = p2.y;
				bb.y_max = m_point.y;
			}
			return bb;
		}

		friend std::ostream& operator<< (std::ostream &out, const Line &Line);

	private:
		Vec2 m_point;
		Vec2 m_direction;
		bool isSegment= false;
	};

	bool operator==(const Line& a, const Line& b)
	{
		if (a.point() == b.point() && a.direction() == b.direction())
			return true;
		return false;
	}

	bool operator!=(const Line& a, const Line& b)
	{
		return !(a == b);
	}

	std::ostream& operator<< (std::ostream &out, const Line &Line)
	{
		out << "Line" << std::endl;
		out << Line.point() << " " << Line.direction() << " " << Line.color() << std::endl;
		return out;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	class Ellipse : public Shape
	{
	public:
		Ellipse(Vec2 origin, Vec2 radius, Color color) :
			Shape(Shape::Derivedtype::ELLIPSE, color),
			m_origin(origin),
			m_radius(radius)
		{};

		//Ellipse specific functions
		const Vec2& origin() const { return (m_origin); }
		const Vec2& radius() const { return(m_radius); }

		//Transformable interface function
		float area() { return(PI * m_radius.x * m_radius.y); }
		float perimeter()
		{ //Ramanujan approx  
			float h = ((m_radius.x - m_radius.y)*(m_radius.x - m_radius.y)) / ((m_radius.x + m_radius.y) * (m_radius.x + m_radius.y));
			float p = PI * (m_radius.x + m_radius.y) * (1 + (3 * h) / (10 + fast_sqrt(4 - 3 * h)));
			return p;
		}
		void homothety(float ratio) { m_radius = ratio*m_radius; }
		void homothety(const Vec2& s, float ratio) 
		{
			Vec2 u = (m_origin - s);
			m_origin = s + ratio*u;
			m_radius = ratio * m_radius;
		}
		void rotate(const Vec2& c, double angle) { /* CANT DO IT */ }
		void rotate(float angle) { /* CANT DO IT (or only pi/2 and pi) */ }
		void translate(const Vec2& t) { m_origin = m_origin + t; }
		void centralSym(const Vec2& c) { Vec2 t = 2 * (c - m_origin); translate(t); }
		void axialSym(const Vec2& p, const Vec2& d)
		{
			Vec2 p1 = p + d;
			Vec2 w = m_origin - p;
			Vec2 vl = p1 - p;
			float b = dot(w, vl) / dot(vl, vl);
			Vec2 intersection = p + b * vl;
			translate(2 * (intersection - m_origin));
		}

		//Displayable interface function
		void display(SDL_Renderer* renderer, float ratio)
		{
			SDL_SetRenderDrawColor(renderer, m_color.r, m_color.g, m_color.b, 0x00);
			int w, h;
			SDL_GetRendererOutputSize(renderer, &w, &h);
			Vec2 center((w / 2), (h / 2));
			Vec2 displayableOrigin = m_origin + center;
			for (int i = -(int)(m_radius.x); i < (int)(m_radius.x); ++i)
			{
				for (int j = -(int)(m_radius.y); j < (int)(m_radius.y); ++j)
				{
					if (j*j*m_radius.x*m_radius.x + i*i*m_radius.y*m_radius.y <= m_radius.x*m_radius.x*m_radius.y*m_radius.y)
					{
						SDL_RenderDrawPoint(renderer, (int)(displayableOrigin.x + i), (int)(displayableOrigin.y + j));
					}
				}
			}
		}

		void serialize(std::string& serial)
		{
			serial = serial + " ellipse";
			serial = serial + " " + to_string(m_origin.x);
			serial = serial + " " + to_string(m_origin.y);
			serial = serial + " " + to_string(m_radius.x);
			serial = serial + " " + to_string(m_radius.y);
			serial = serial + " " + to_string(m_color.r);
			serial = serial + " " + to_string(m_color.g);
			serial = serial + " " + to_string(m_color.b);
		}

		BoundingBox bounding_box()
		{
			BoundingBox bb;
			bb.x_min = (int)(m_origin.x - m_radius.x);
			bb.x_max = (int)(m_origin.x + m_radius.x);
			bb.y_min = (int)(m_origin.y - m_radius.y);
			bb.y_max = (int)(m_origin.y + m_radius.y);

			return bb;
		}

		friend std::ostream& operator<< (std::ostream &out, const Ellipse &Ellipse);

	private:
		Vec2 m_origin;
		Vec2 m_radius;
	};

	bool operator==(const Ellipse& a, const Ellipse& b)
	{
		if (a.origin() == b.origin() && a.radius() == b.radius())
			return true;
		return false;
	}

	bool operator!=(const Ellipse& a, const Ellipse& b)
	{
		return !(a == b);
	}

	std::ostream& operator<< (std::ostream &out, const Ellipse &Ellipse)
	{
		out << "Ellipse" << std::endl;
		out << Ellipse.origin() << " " << Ellipse.radius() << " " << Ellipse.color() << std::endl;
		return out;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////

	class Image : public Shape
	{
	public:
		Image(Vec2 o = { 0, 0 }) : Shape(Shape::IMAGE, Color(0, 0, 0)), annotation(std::string()), components_(std::vector<Shape *>()), origin_(o){}
		Image(const Image&) = delete;
		Image& operator=(Image const&) = delete;
		
		float area()
		{			
			BoundingBox bb = bounding_box();
			std::lock_guard<std::mutex> guard(mutex);
			int w = bb.x_max - bb.x_min;
			int h = bb.y_max - bb.y_min;
			return  (float)w*h;
		}

		float perimeter()
		{
			BoundingBox bb = bounding_box();
			std::lock_guard<std::mutex> guard(mutex);
			int w = bb.x_max - bb.x_min;
			int h = bb.y_max - bb.y_min;
			return 2.f*(w+h);
		}

		void translate(const Vec2& t)
		{
			std::lock_guard<std::mutex> guard(mutex);
			for (auto component : components_)
			{
				component->translate(t);
			}
		}

		void homothety(float ratio)
		{
			std::lock_guard<std::mutex> guard(mutex);
			for (auto component : components_)
			{
				component->homothety(ratio);
			}
		}

		void homothety(const Vec2& s, float ratio)
		{
			std::lock_guard<std::mutex> guard(mutex);
			for (auto component : components_)
			{
				component->homothety(s, ratio);
			}
		}

		void rotate(float angle)
		{
			std::lock_guard<std::mutex> guard(mutex);
			for (auto component : components_)
			{
				component->rotate(angle);
			}
		}

		void rotate(const Vec2& p, double angle)
		{
			std::lock_guard<std::mutex> guard(mutex);
			for (auto component : components_)
			{
				component->rotate(p, angle);
			}
		}
		
		void centralSym(const Vec2& c)
		{
			std::lock_guard<std::mutex> guard(mutex);
			for (auto component : components_)
			{
				component->centralSym(c);
			}
		}

		void axialSym(const Vec2& p, const Vec2& d)
		{
			std::lock_guard<std::mutex> guard(mutex);
			for (auto component : components_)
			{
				component->axialSym(p, d);
			}
		}
		
		BoundingBox bounding_box()
		{
			std::lock_guard<std::mutex> guard(mutex);
			BoundingBox bb_ = {};
			BoundingBox bb = {};
			for (auto component : components_)
			{
				bb = component->bounding_box();
				if (bb_.x_max < bb.x_max)
					bb_.x_max = bb.x_max;
				if (bb_.x_min > bb.x_min)
					bb_.x_min = bb.x_min;
				if (bb_.y_max < bb.y_max)
					bb_.y_max = bb.y_max;
				if (bb_.y_min > bb.y_min)
					bb_.y_min = bb.y_min;
			}
			return bb_;
		}
		
		void add_component(Shape* s)
		{ 
			std::lock_guard<std::mutex> guard(mutex);
			s->translate(origin_);
			components_.push_back(s); 
		}

		Vec2 origin() const
		{
			return origin_;
		}

		void origin( Vec2& new_origin)
		{
			Vec2 v = (origin_ - new_origin);
			translate(v);
			origin_ = new_origin;
		}

		void display(SDL_Renderer* renderer, float ratio)
		{
			std::lock_guard<std::mutex> guard(mutex);
			for (auto component : components_)
			{
				component->display(renderer, ratio);
			}
		}

		void display(SDL_Renderer* renderer)
		{
			BoundingBox bb = bounding_box();
			std::lock_guard<std::mutex> guard(mutex);

			int w, h;
			SDL_GetRendererOutputSize(renderer, &w, &h);
			Vec2 center((w / 2), (h / 2));
			bb.x_max = bb.x_max + center.x;
			bb.x_min = bb.x_min + center.x;
			bb.y_max = bb.y_max + center.y;
			bb.y_min = bb.y_min + center.y;
			Vec2 v1 = (center - Vec2( bb.x_max, bb.y_max ));
			Vec2 v2 = (center - Vec2(bb.x_min, bb.y_min));
			int im_w, im_h;
			float n1 = norm(v1);
			float n2 = norm(v2);
			if (norm(v1) > norm(v2))
			{
				im_w = n1 * 2;
				im_h = im_w;
			}
			else
			{
				im_w = n2 * 2;
				im_h = im_w;
			}

			float w_ratio = (float) w / im_w;
			float h_ratio = (float) h / im_h;
			float final_ratio = 1.f;

			if (w_ratio < 1.f || h_ratio < 1.f)
			{
				if (w_ratio <= h_ratio)
				{
					final_ratio = w_ratio;
				}
				else
				{
					final_ratio = h_ratio;
				}
			}

			for (auto component : components_)
			{

				component->display(renderer, final_ratio);
			}
		}

		std::string get_annotation()
		{
			std::lock_guard<std::mutex> guard(mutex);
			return annotation;
		}

		void annotate(std::string msg)
		{
			std::lock_guard<std::mutex> guard(mutex);
			annotation = msg;
		}

		void serialize(std::string& serial)
		{
			std::lock_guard<std::mutex> guard(mutex);
			int max_x = 0, max_y = 0;
			for (auto component : components_)
			{
				component->serialize(serial);
			}
			serial = serial + " annotation " + to_string((int)annotation.size()) + " " + annotation;
		}

		void deserialize(std::string s)
		{
			components_.clear();
			std::istringstream buf(s);
			for (std::string word; buf >> word;)
			{
				switch (Shape::ShapeStringToEnum(word))
				{

				}
				if (word == "circle")
				{
					buf >> word;
					float x = std::stof(word);
					buf >> word;
					float y = std::stof(word);
					buf >> word;
					float rad = std::stof(word);
					buf >> word;
					int r = std::stoi(word);
					buf >> word;
					int g = std::stoi(word);
					buf >> word;
					int b = std::stoi(word);
					add_component(new Circle(Vec2(x, y), rad, Color(r, g, b)));
				}

				else if (word == "polygon")
				{
					std::vector<Vec2> points;
					buf >> word;
					int nb_pts = std::stoi(word);
					for (int i = 0; i < nb_pts; i++)
					{
						buf >> word;
						float x = std::stof(word);
						buf >> word;
						float y = std::stof(word);
						points.push_back(Vec2(x, y));
					}
					buf >> word;
					int r = std::stoi(word);
					buf >> word;
					int g = std::stoi(word);
					buf >> word;
					int b = std::stoi(word);
					add_component(new Polygon(points, Color(r, g, b)));
				}

				else if (word == "line")
				{
					buf >> word;
					float x = std::stof(word);
					buf >> word;
					float y = std::stof(word);
					buf >> word;
					float dir_x = std::stof(word);
					buf >> word;
					float dir_y = std::stof(word);
					buf >> word;
					int r = std::stoi(word);
					buf >> word;
					int g = std::stoi(word);
					buf >> word;
					int b = std::stoi(word);
					add_component(new Line(Vec2(x, y), Vec2(dir_x, dir_y), Color(r, g, b)));
				}

				else if (word == "ellipse")
				{
					buf >> word;
					float x = std::stof(word);
					buf >> word;
					float y = std::stof(word);
					buf >> word;
					float rad_x = std::stof(word);
					buf >> word;
					float rad_y = std::stof(word);
					buf >> word;
					int r = std::stoi(word);
					buf >> word;
					int g = std::stoi(word);
					buf >> word;
					int b = std::stoi(word);
					add_component(new Ellipse(Vec2(x, y), Vec2(rad_x, rad_y), Color(r, g, b)));
				}

				else if (word == "annotation")
				{
					buf >> word;
					int string_size = std::stoi(word);
					char buffer[1024];
					buf.getline(buffer, string_size + 2);
					std::string annotation = std::string(buffer);
					annotate(annotation);
				}
			}
		}

		std::vector< Shape* >& components()
		{
			std::lock_guard<std::mutex> guard(mutex);
			return components_;
		}

	private:
		std::vector< Shape* > components_;
		std::string annotation;
		std::mutex mutex;
		Vec2 origin_;
	};


	///////////////////////////////////////////////////////////////////////////////

	std::ostream& operator<< (std::ostream &out, Shape &Shape)
	{
		switch (Shape.type())
		{
			case Shape::Derivedtype::CIRCLE:
			{
				Circle* c = static_cast<Circle*>(&Shape);
				out << *c;
			}break;
			case Shape::Derivedtype::POLYGON:
			{
				Polygon* p = static_cast<Polygon*>(&Shape);
				out << *p;
			}break;
			case Shape::Derivedtype::ELLIPSE:
			{
				Ellipse* e = static_cast<Ellipse*>(&Shape);
				out << *e;
			}break;
			case Shape::Derivedtype::LINE:
			{
				Line* l = static_cast<Line*>(&Shape);
				out << *l;
			}break;
		}

		return out;
	}

}
