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

class chat_client
{
public:
  chat_client(boost::asio::io_service& io_service,
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
			  std::cout << "Received server connection" << std::endl;
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
			  std::cout << "Received msg from serv" << std::endl;
			  if (std::string(read_msg_.body()) == "TEST")
			  {
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
				  img = deserialize(std::string(read_msg_.body()));
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

int _tmain(int argc, _TCHAR* argv[])
{
  try
  {
    if (argc != 3)
    {
      std::cerr << "Usage: chat_client <host> <port>\n";
      //return 1;
    }

	std::vector<std::string> functions = { "rotate", "homothety", "translate", "axial_sym", "central_sym" };
	std::vector<std::string> types = { "circle", "polygon", "line", "ellipse" };

	Image image;
	image.add_component(new Circle(Vec2(400, 300), 50, Color(255, 0, 0)));
	image.add_component(new Patchwork::Polygon({ { 200, 200 }, { 300, 200 }, { 300, 400 }, { 200, 400 } }, Color(0, 255, 0)));
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Event event;
	SDL_Window *window;
	SDL_Renderer *renderer;
	
    boost::asio::io_service io_service;

    tcp::resolver resolver(io_service);
    //auto endpoint_iterator = resolver.resolve({ (char*)argv[1], (char*)argv[2] });
	auto endpoint_iterator = resolver.resolve({ "127.0.0.1", "8080" });
    chat_client c(io_service, endpoint_iterator, image);

    std::thread t([&io_service](){ io_service.run(); });

    char line[chat_message::max_body_length + 1];
	std::string line_str;
    while (std::cin.getline(line, chat_message::max_body_length + 1))
    {
		line_str = std::string(line);
		if (line_str == "display")
		{
			std::cout << "Annotation : " << image.get_annotation() << std::endl;
			SDL_CreateWindowAndRenderer(800, 600, 0, &window, &renderer);
			while (1) {
				SDL_PollEvent(&event);
				if (event.type == SDL_QUIT) {
					break;
				}
				SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0x00);
				SDL_RenderClear(renderer);
				image.display(renderer);
				SDL_RenderPresent(renderer);
			}
			SDL_DestroyWindow(window);
		}

		if (line_str == "annotation")
		{
			image.get_annotation();
		}

		if (line_str == "make")
		{
			std::cout << "Type ? " << std::endl;
			std::string buf;
			std::cin >> buf;
			if (buf == "circle")
			{
				float x;
				std::cout << "Origin x : ";
				std::cin >> x;
				float y;
				std::cout << "Origin y : ";
				std::cin >> y;
				float radius;
				std::cout << "Radius : ";
				std::cin >> radius;
				int r, g, b;
				std::cout << "Color R : ";
				std::cin >> r;
				std::cout << "Color G : ";
				std::cin >> g;
				std::cout << "Color B : ";
				std::cin >> b;
				image.add_component(new Circle(Vec2(x, y), radius, Color(r, g, b)));
			}
			else if (buf == "polygon")
			{
				int nb_pts;
				std::vector<Vec2> points;
				std::cout << "Nombre de sommets : ";
				std::cin >> nb_pts;

				for (int i = 0; i < nb_pts; ++i)
				{
					float x;
					std::cout << "Origin x : ";
					std::cin >> x;
					float y;
					std::cout << "Origin y : ";
					std::cin >> y;
					points.push_back(Vec2(x, y));
				}
				int r, g, b;
				std::cout << "Color R : ";
				std::cin >> r;
				std::cout << "Color G : ";
				std::cin >> g;
				std::cout << "Color B : ";
				std::cin >> b;
				image.add_component(new Patchwork::Polygon(points, Color(r,g,b)));
			}
			else if (buf == "ellipse")
			{
				float x;
				std::cout << "Origin x : ";
				std::cin >> x;
				float y;
				std::cout << "Origin y : ";
				std::cin >> y;
				float rad_x;
				std::cout << "Radius x : ";
				std::cin >> rad_x;
				float rad_y;
				std::cout << "Radius y : ";
				std::cin >> rad_y;
				int r, g, b;
				std::cout << "Color R : ";
				std::cin >> r;
				std::cout << "Color G : ";
				std::cin >> g;
				std::cout << "Color B : ";
				std::cin >> b;
				image.add_component(new Patchwork::Ellipse(Vec2(x,y), Vec2(rad_x, rad_y), Color(r, g, b)));
			}
			else if (buf == "line")
			{
				float x;
				std::cout << "Origin x : ";
				std::cin >> x;
				float y;
				std::cout << "Origin y : ";
				std::cin >> y;
				float rad_x;
				std::cout << "Vector x : ";
				std::cin >> rad_x;
				float rad_y;
				std::cout << "Vector y : ";
				std::cin >> rad_y;
				int r, g, b;
				std::cout << "Color R : ";
				std::cin >> r;
				std::cout << "Color G : ";
				std::cin >> g;
				std::cout << "Color B : ";
				std::cin >> b;
				image.add_component(new Patchwork::Line(Vec2(x, y), Vec2(rad_x, rad_y), Color(r, g, b)));
			}
			else
			{
				std::cout << "Type unknown" << std::endl;
			}			
		}

		if (line_str == "send")
		{
			chat_message msg;
			std::string serial;
			image.serialize(serial);
			msg.body_length(serial.size());
			std::memcpy(msg.body(), serial.c_str(), msg.body_length());
			msg.encode_header();
			c.write(msg);
		}

		if (line_str == "transform")
		{
			int id;
			std::cout << "ID ?" << std::endl;
			std::cin >> id;

			
		}

		if (line_str == "print")
		{
			for (int i = 0; i < image.components().size(); i++)
			{
				std::cout << i << " " << *image.components().at(i);
			}
		}

		if (line_str == "delete")
		{
			int id;
			std::cout << "ID ?" << std::endl;
			std::cin >> id;
		}
    }

    c.close();
    t.join();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
