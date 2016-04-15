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
#include <tchar.h>
#include "chat_message.hpp"
#include "Shape.h"

using boost::asio::ip::tcp;
using namespace Patchwork;

typedef std::deque<chat_message> chat_message_queue;

class ClientIO
{
public:
  ClientIO(boost::asio::io_service& io_service,
      tcp::resolver::iterator endpoint_iterator,
	  Image& img)
    : io_service_(io_service),
      socket_(io_service),
	  img(img)
  {
    do_connect(endpoint_iterator);
  }

  void write(const chat_message& msg)
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

  void close()
  {
    io_service_.post([this]() { socket_.close(); });
  }

private:
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

  void do_read_header()
  {
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.data(), chat_message::header_length),
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
				  chat_message msg;
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
  boost::asio::io_service& io_service_;
  tcp::socket socket_;
  chat_message read_msg_;
  chat_message_queue write_msgs_;
  Image& img;
};


class Client
{
public :
	const enum Commands { DISPLAY = 0, MAKE, TRANSFORM, PRINT, SEND, DELETE_, HELP, UNKNOWN };
	static const std::vector<std::string> cmds;

	static void print_commands()
	{
		for (auto cmd : cmds)
			std::cout << " " << cmd;
	}

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

	Client(std::string ip, std::string port, boost::asio::io_service& service) : io_service(service)
	{
		img = new Image();
		resolver = new tcp::resolver(io_service);
		auto endpoint_iterator = resolver->resolve({ "127.0.0.1", "8080" });
		c = new ClientIO(io_service, endpoint_iterator, *img);
		std::thread* t = new std::thread([&](){ io_service.run(); });
		SDL_Init(SDL_INIT_VIDEO);
		start_polling();
	};

private:
	void start_polling()
	{
		const unsigned int LINE_MAX_SIZE = 256;
		// Start polling for commands
		char line[LINE_MAX_SIZE];
		std::string cmd;
		//    While the users is entering commands we react to it
		while (std::cin.getline(line, LINE_MAX_SIZE))
		{
			cmd = std::string(line);

			switch (CmdStringToEnum(cmd))
			{
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
					std::cout << "available commands : ";
					print_commands();
				}break;

				case Commands::MAKE:
				{
					std::cout << "Available shapes : ";
					Shape::print_shapes();
					std::cout << "Enter Shape Type : ";
					std::cin >> cmd;
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
								img->add_component(new Circle(Vec2(x, y), radius, Color(r, g, b)));
							}
							catch (std::exception& e)
							{
								//Catch error of types when cin trying to convert to desired type
								std::cout << " Problem : " << e.what() << std::endl;
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
								img->add_component(new Patchwork::Ellipse(Vec2(x, y), Vec2(rad_x, rad_y), Color(r, g, b)));
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
								img->add_component(new Patchwork::Line(Vec2(x, y), Vec2(dir_x, dir_y), Color(r, g, b)));
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
								img->add_component(new Patchwork::Polygon(points, Color(r, g, b)));
							}
							catch (std::exception& e)
							{
								//Catch error of types when cin trying to convert to desired type
								std::cout << " Problem : " << e.what() << std::endl;
							}
						}break;

						default:
						{
							std::cout << "Unknown command" << std::endl;
						}break;
					}
				}break;

				case Commands::SEND:
				{
					chat_message msg;
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
					print_components();
					std::cout << "Choose a shape ID : ";
					std::cin >> id;
					std::cout << "Available transforms : ";
					Shape::print_transforms();
					std::cout << "Choose a transformation : ";
					std::cin >> buf;
					switch (Shape::FuncStringToEnum(buf))
					{
						case Shape::HOMOTHETY:
						{
							float ratio;
							std::cout << "Ratio : " << std::endl;
							std::cin >> ratio;
							img->components().at(id)->homothety(ratio);
						}break;

						case Shape::AXIAL_SYMETRY:
						{
							float x, y, dir_x, dir_y;
							std::cout << "Axe point X : " << std::endl;
							std::cin >> x;
							std::cout << "Axe point Y : " << std::endl;
							std::cin >> y;
							std::cout << "Axe direction X : " << std::endl;
							std::cin >> dir_x;
							std::cout << "Axe direction Y : " << std::endl;
							std::cin >> dir_y;
							img->components().at(id)->axialSym(Vec2(x, y), Vec2(dir_x, dir_y));
						}break;

						case Shape::CENTRAL_SYMETRY:
						{
							float x, y;
							std::cout << "Center X : " << std::endl;
							std::cin >> x;
							std::cout << "Center Y : " << std::endl;
							std::cin >> y;
							img->components().at(id)->centralSym(Vec2(x, y));
						}break;

						case Shape::ROTATION:
						{
							float angle;
							std::cout << "Angle : " << std::endl;
							std::cin >> angle;
							img->components().at(id)->rotate(angle);
						}break;

						case Shape::TRANSLATE:
						{
							float x, y;
							std::cout << "Translation X : " << std::endl;
							std::cin >> x;
							std::cout << "Translation Y : " << std::endl;
							std::cin >> y;
							img->components().at(id)->translate(Vec2(x, y));
						}break;

						default:
						{
							std::cout << "Unknown transformation" << std::endl;
						}
					}
				}break;

				case Commands::PRINT:
				{
					print_components();
				}break;

				case Commands::DELETE_:
				{
					try
					{
						int id;
						print_components();
						std::cout << "Enter ID : " << std::endl;
						std::cin >> id;
						auto it = img->components().begin();
						img->components().erase(it);
					}
					catch (std::exception& e)
					{
						std::cout << "Unknown ID" << std::endl;
					}
				}break;

				default:
				{
					std::cout << "Unknow command" << std::endl;
				}
			}
		}
		c->close();
		t->join();
	}

	void print_components()
	{
		for (int i = 0; i < img->components().size(); i++)
		{
			std::cout << i << " " << *img->components().at(i);
		}
	}

	ClientIO* c;
	SDL_Event event;
	SDL_Window *window;
	SDL_Renderer *renderer;
	boost::asio::io_service& io_service;
	tcp::resolver* resolver;
	std::thread* t;
	Image* img;
};
const std::vector<std::string> Client::cmds = { "display", "make", "transform", "print", "send", "delete" , "help"};

int _tmain(int argc, _TCHAR* argv[])
{
  boost::asio::io_service io_service;
  Client c("127.0.0.1", "8080", io_service);
  return 0;
}
