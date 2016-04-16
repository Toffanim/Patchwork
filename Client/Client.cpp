//
// chat_client.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <deque>
#include <iostream>
#include <thread>
#include <boost/asio.hpp>
#include <stdio.h>
#if _WIN32
#include <tchar.h>
#endif
#include "Message.hpp"
#include "Shape.h"

using boost::asio::ip::tcp;
using namespace Patchwork;

typedef std::deque<Message> Message_queue;

/*! \file Client.cpp
\brief File containing the client part of the application

*/

/*!
Class that handle the input and output of the client (basically reading and writing to the socket)
*/
class ClientIO
{
public:
	/*!
	Class that handle the input and output of the client (basically reading and writing to the socket).
	This class is based an asynchronous IO pattern (c.f boost::asio).
	\param io_service The boost::asio io_service providing event polling on the socket
	\param endpoint_iterator The boost::asio TCP iterator
	\param img Reference to the image currently owned by the Client (so we can send it)
	*/
  ClientIO(boost::asio::io_service& io_service,
      tcp::resolver::iterator endpoint_iterator,
	  Image& img)
    : io_service_(io_service),
      socket_(io_service),
	  img(img)
  {
	  //Check for connection
    do_connect(endpoint_iterator);
  }
  /*!
  Tells the socket that we want to write a message
  \param msg the message to send
  */
  void write(const Message& msg)
  {
    io_service_.post(
        [this, msg]()
        {
          bool write_in_progress = !write_msgs_.empty();
          write_msgs_.push_back(msg);
          if (!write_in_progress)
          {
            do_write();
          }
        });
  }
  /*!
  Tells the socket that we want to close the connection
  */
  void close()
  {
    io_service_.post([this]() { socket_.close(); });
  }

private:
	/*!
	Resolve the external connection to the socket
	When a connection is find, the handler will start reading the message
	*/
  void do_connect(tcp::resolver::iterator endpoint_iterator)
  {
    boost::asio::async_connect(socket_, endpoint_iterator,
        [this](boost::system::error_code ec, tcp::resolver::iterator)
        {
          if (!ec)
          {
            do_read_header();
          }
        });
  }
  /*!
  Read from the socket into a buffer and analyze our message header, then ask to read the message's body
  */
  void do_read_header()
  {
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.data(), Message::header_length),
        [this](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec && read_msg_.decode_header())
          {
            do_read_body();
          }
          else
          {
            socket_.close();
          }
        });
  }
  /*!
  Read from the socket into a buffer and analyze our message body, then start again to read from the socket is some reads are needed to be done (due to asynchronous design)
  */
  void do_read_body()
  {
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
        [this](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
			  if (std::string(read_msg_.body()) == "GET")
			  {
				  //Send image
				  Message msg;
				  std::string s;
				  img.serialize(s);
				  msg.body_length(s.length());
				  std::memcpy(msg.body(), s.c_str(), msg.body_length());
				  msg.encode_header();
				  write(msg);
			  }
			  else
			  {
				  //Get image
				  img.deserialize(std::string(read_msg_.body()));
			  }
            do_read_header();
          }
          else
          {
            socket_.close();
          }
        });
  }
  /*!
  Write to the socket, then ask to write again if some writes are needed to be done (due to asychronous design)
  */
  void do_write()
  {
    boost::asio::async_write(socket_,
        boost::asio::buffer(write_msgs_.front().data(),
          write_msgs_.front().length()),
        [this](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
            write_msgs_.pop_front();
            if (!write_msgs_.empty())
            {
              do_write();
            }
          }
          else
          {
            socket_.close();
          }
        });
  }

private:
  boost::asio::io_service& io_service_; /*!< boost::asio IO service */
  tcp::socket socket_; /*!< boost::asio TCP Socket */
  Message read_msg_; /*!< Message read from the socket */
  Message_queue write_msgs_; /*!< Queue of messages to be sent */
  Image& img; /*!< REference to the image currently owned by the Client */
};

/*!
Class that handle the Client's input commands, basically polling commands from the console and reacting to it
*/
class Client
{
public :
	enum Commands { DISPLAY = 0, MAKE, TRANSFORM, PRINT, SEND, DELETE_, HELP, QUIT, UNKNOWN }; /*!< Enums of available commands */
	static const std::vector<std::string> cmds; /*!< A static container of strings defining the command string assiciaited to its Commands enum value  */
	/*!
	Static function to print available commands keywords
	*/
	static void print_commands()
	{
		for (auto cmd : cmds)
			std::cout << " " << cmd;
	}
	/*!
	Function to convert a string into a Command enum.
	Return UNKNOWN if not in the container.
	*/
	Commands CmdStringToEnum(std::string s)
	{
		for (int i = 0; i < cmds.size(); ++i)
		{
			if (cmds.at(i) == s)
			{
				return static_cast<Commands>(i);
			}
		}
		return Commands::UNKNOWN;
	}	
	/*!
	Class that creates the ClientIO and poll user input to execute commands
	\param ip TCP Socket IP
	\param port TCP Socket port
	\param service boost::asio io_service
	*/
	Client(std::string ip, std::string port, boost::asio::io_service& service) : io_service(service)
	{
		img = new Image();
		//Initiliaze connection
		resolver = new tcp::resolver(io_service);
		auto endpoint_iterator = resolver->resolve({ "127.0.0.1", "8080" });
		c = new ClientIO(io_service, endpoint_iterator, *img);
		t = new std::thread([&](){ io_service.run(); });
		SDL_Init(SDL_INIT_VIDEO);
		start_polling();
	};

	~Client()
	{
		delete img;
		delete resolver;
		delete c;
		t->join();
		SDL_Quit();
	}

private:
	/*!
	Function thats polls user inputs and call the associated functions
	*/
	void start_polling()
	{
		bool quit = false;
		const unsigned int LINE_MAX_SIZE = 256;
		// Start polling for commands
		char line[LINE_MAX_SIZE];
		std::string cmd;
		//    While the users is entering commands we react to it
		std::cout << "Available commands : ";
		print_commands();
		std::cout << std::endl << "Command : ";
		while (std::cin.getline(line, LINE_MAX_SIZE))
		{
			cmd = std::string(line);
			// Convert string to a command enum we can switch on
			switch (CmdStringToEnum(cmd))
			{
				case Commands::QUIT:
				{
					quit = true;
				}break;

				case Commands::DISPLAY:
				{
					//Print annotation in console
					std::cout << "Annotation : " << img->get_annotation() << std::endl;
					//Create window and display image
					SDL_CreateWindowAndRenderer(800, 600, 0, &window, &renderer);
					while (1) {
						SDL_PollEvent(&event);
						if (event.type == SDL_QUIT) {
							break;
						}
						SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0x00);
						SDL_RenderClear(renderer);
						img->display(renderer);
						SDL_RenderPresent(renderer);
					}
					SDL_DestroyWindow(window);
				}break;

				case Commands::HELP:
				{
					//Print available commands
					std::cout << "available commands : ";
					print_commands();
				}break;

				case Commands::MAKE:
				{
					//Print available shapes
					std::cout << "Available shapes : ";
					Shape::print_shapes();
					std::cout << std::endl;
					std::cout << "Enter Shape Type : ";
					std::cin >> cmd;
					//Switch on user entered shapes
					switch (Shape::ShapeStringToEnum(cmd))
					{
						case Shape::Derivedtype::CIRCLE:
						{
							try
							{
								float x, y, radius;
								int r, g, b;
								std::cout << "Origin x : ";
								std::cin >> x;
								std::cout << "Origin y : ";
								std::cin >> y;
								std::cout << "Radius : ";
								std::cin >> radius;
								std::cout << "Color R : ";
								std::cin >> r;
								std::cout << "Color G : ";
								std::cin >> g;
								std::cout << "Color B : ";
								std::cin >> b;
								if (std::cin.fail())
								{
									std::cin.clear();
									throw std::domain_error("Bad input");
								}
								img->add_component(new Circle(Vec2(x, y), radius, Color(r, g, b)));
								std::cout << "Circle created" << std::endl;
							}
							catch (std::exception& e)
							{
								//Catch error of types when cin trying to convert to desired type
								std::cout << std::endl << " Problem : " << e.what() << std::endl;
							}				
						}break;

						case Shape::Derivedtype::ELLIPSE:
						{
							try
							{
								float x , y , rad_x, rad_y;
								int r, g, b;
								std::cout << "Origin x : ";
								std::cin >> x;
								std::cout << "Origin y : ";
								std::cin >> y;
								std::cout << "Radius x : ";
								std::cin >> rad_x;
								std::cout << "Radius y : ";
								std::cin >> rad_y;
								std::cout << "Color R : ";
								std::cin >> r;
								std::cout << "Color G : ";
								std::cin >> g;
								std::cout << "Color B : ";
								std::cin >> b;
								if (std::cin.fail())
								{
									std::cin.clear();
									throw std::domain_error("Bad input");
								}
								img->add_component(new Patchwork::Ellipse(Vec2(x, y), Vec2(rad_x, rad_y), Color(r, g, b)));
								std::cout << "Ellipse created" << std::endl;
							}
							catch (std::exception& e)
							{
								//Catch error of types when cin trying to convert to desired type
								std::cout << " Problem : " << e.what() << std::endl;
							}
						}break;

						case Shape::Derivedtype::LINE:
						{
							try
							{
								float x, y, dir_x, dir_y;
								int r, g, b;
								std::cout << "Origin x : ";
								std::cin >> x;
								std::cout << "Origin y : ";
								std::cin >> y;
								std::cout << "Vector x : ";
								std::cin >> dir_x;
								std::cout << "Vector y : ";
								std::cin >> dir_y;
								std::cout << "Color R : ";
								std::cin >> r;
								std::cout << "Color G : ";
								std::cin >> g;
								std::cout << "Color B : ";
								std::cin >> b;
								if (std::cin.fail())
								{
									std::cin.clear();
									throw std::domain_error("Bad input");
								}
								img->add_component(new Patchwork::Line(Vec2(x, y), Vec2(dir_x, dir_y), Color(r, g, b)));
								std::cout << "Line created" << std::endl;
							}
							catch (std::exception& e)
							{
								//Catch error of types when cin trying to convert to desired type
								std::cout << " Problem : " << e.what() << std::endl;
							}
						}break;

						case Shape::POLYGON:
						{
							try
							{
								int nb_pts, r, g, b;
								std::vector<Vec2> points;
								std::cout << "Vertex count : ";
								std::cin >> nb_pts;

								float x, y;
								for (int i = 0; i < nb_pts; ++i)
								{
									std::cout << "Origin x : ";
									std::cin >> x;
									std::cout << "Origin y : ";
									std::cin >> y;
									points.push_back(Vec2(x, y));
								}

								std::cout << "Color R : ";
								std::cin >> r;
								std::cout << "Color G : ";
								std::cin >> g;
								std::cout << "Color B : ";
								std::cin >> b;
								if (std::cin.fail())
								{
									std::cin.clear();
									throw std::domain_error("Bad input");
								}
								img->add_component(new Patchwork::Polygon(points, Color(r, g, b)));
								std::cout << "Polygon created" << std::endl;
							}
							catch (std::exception& e)
							{
								//Catch error of types when cin trying to convert to desired type
								std::cout << " Problem : " << e.what() << std::endl;
							}
						}break;

						default:
						{
							std::cout << "Unknown shape" << std::endl;
						}break;
					}
				}break;

				case Commands::SEND:
				{
					// Send the image to the server
					Message msg;
					std::string serial;
					img->serialize(serial);
					msg.body_length(serial.size());
					std::memcpy(msg.body(), serial.c_str(), msg.body_length());
					msg.encode_header();
					c->write(msg);
				}break;

				case Commands::TRANSFORM:
				{
					int id;
					std::string buf;
					try
					{
						//Transform an existing shape	
						print_components();
						std::cout << std::endl;
						std::cout << "Choose a shape ID : ";
						std::cin >> id;
						if (std::cin.fail())
						{
							std::cin.clear();
							throw std::domain_error("Bad input");
						}
					}
					catch (std::exception& e)
					{
						std::cout << std::endl << "Problem : " << e.what() << std::endl;
						break;
					}
					std::cout << "Available transforms : ";
					Shape::print_transforms();
					std::cout << std::endl;
					std::cout << "Choose a transformation : ";
					std::cin >> buf;
					switch (Shape::FuncStringToEnum(buf))
					{
						case Shape::HOMOTHETY:
						{
							try
							{
								float ratio;
								std::cout << "Ratio : ";
								std::cin >> ratio;
								if (std::cin.fail())
								{
									std::cin.clear();
									throw std::domain_error("Bad input");
								}
								img->components().at(id)->homothety(ratio);
							}
							catch (std::exception& e)
							{
								std::cout << std::endl << "Problem : " << e.what() << std::endl;
							}
						}break;

						case Shape::AXIAL_SYMETRY:
						{
							try
							{
								float x, y, dir_x, dir_y;
								std::cout << "Axe point X : ";
								std::cin >> x;
								std::cout << "Axe point Y : ";
								std::cin >> y;
								std::cout << "Axe direction X : ";
								std::cin >> dir_x;
								std::cout << "Axe direction Y : ";
								std::cin >> dir_y;
								if (std::cin.fail())
								{
									std::cin.clear();
									throw std::domain_error("Bad input");
								}
								img->components().at(id)->axialSym(Vec2(x, y), Vec2(dir_x, dir_y));
							}
							catch (std::exception& e)
							{
								std::cout << std::endl << "Problem : " << e.what() << std::endl;
							}
						}break;

						case Shape::CENTRAL_SYMETRY:
						{
							try
							{
								float x, y;
								std::cout << "Center X : ";
								std::cin >> x;
								std::cout << "Center Y : ";
								std::cin >> y;
								if (std::cin.fail())
								{
									std::cin.clear();
									throw std::domain_error("Bad input");
								}
								img->components().at(id)->centralSym(Vec2(x, y));
							}
							catch (std::exception& e)
							{
								std::cout << std::endl << "Problem : " << e.what() << std::endl;
							}
						}break;

						case Shape::ROTATION:
						{
							try
							{
								float angle;
								std::cout << "Angle (Degree) : ";
								std::cin >> angle;
								if (std::cin.fail())
								{
									std::cin.clear();
									throw std::domain_error("Bad input");
								}
								img->components().at(id)->rotate(DEGTORAD*angle);
							}
							catch (std::exception& e)
							{
								std::cout << std::endl << "Problem : " << e.what() << std::endl;
							}
						}break;

						case Shape::TRANSLATE:
						{
							try
							{
								float x, y;
								std::cout << "Translation X : ";
								std::cin >> x;
								std::cout << "Translation Y : ";
								std::cin >> y;
								if (std::cin.fail())
								{
									std::cin.clear();
									throw std::domain_error("Bad input");
								}
								img->components().at(id)->translate(Vec2(x, y));
							}
							catch (std::exception& e)
							{
								std::cout << std::endl << "Problem : " << e.what() << std::endl;
							}
						}break;

						default:
						{
							std::cout << "Unknown transformation" << std::endl;
						}
					}
				}break;

				case Commands::PRINT:
				{
					// Print componentns of the image
					print_components();
				}break;

				case Commands::DELETE_:
				{
					//Delete one component
					try
					{
						int id;
						print_components();
						std::cout << std::endl;
						std::cout << "Enter ID : " ;
						std::cin >> id;
						if (std::cin.fail())
						{
							std::cin.clear();
							throw std::domain_error("Bad input");
						}
						auto tmp = img->components().at( id ); // test to throw exception
						auto it = img->components().begin() + id;
						img->components().erase(it);
					}
					catch (std::exception& e)
					{
						std::cout << "Unknown ID" << std::endl;
					}
				}break;

				default:
				{
					if (!cmd.empty())
					    std::cout << "Unknown command" << std::endl;
				}
			}

			if (quit)
				break;

			std::cin.clear();
			std::cin.ignore(100000, '\n');
			std::cout << std::endl << "Command : ";
		}
		c->close();
		io_service.stop();
		t->join();
	}
	/*!
	Print out the image componentns
	*/
	void print_components()
	{
		if (img->components().size())
		{
			for (int i = 0; i < img->components().size(); i++)
			{
				std::cout << i << " " << *img->components().at(i);
			}
		}
		else
		{
			std::cout << "No components" << std::endl;
		}
	}

	ClientIO* c; /*!< The Client Input/Output on socket */
	SDL_Event event; /*!< SDL Event so we can know when to close the window */
	SDL_Window *window; /*!< SDL window to display to */
	SDL_Renderer *renderer; /*!< SDL renderer to draw components to */
	boost::asio::io_service& io_service; /*!< boost::asio io_service */
	tcp::resolver* resolver; /*!< boost::asio TCP resolver */
	std::thread* t; /*!< Thread polling Input/Output event from io_service */
	Image* img; /*!< Image being created by the client */
};
const std::vector<std::string> Client::cmds = { "display", "make", "transform", "print", "send", "delete" , "help", "quit"};

#if _WIN32
int _tmain(int argc, _TCHAR* argv[])
#else
int main()
#endif
{
	//Create io_service and start Client
  boost::asio::io_service io_service;
  //Client will be cleaned by app
  Client c("127.0.0.1", "8080", io_service);
  return 0;
}
