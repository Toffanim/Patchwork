//
// chat_server.cpp
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
#include <list>
#include <memory>
#include <set>
#include <thread>
#include <utility>
#include <stdio.h>
#if _WIN32_
#include <tchar.h>
#endif
#include <boost/asio.hpp>
#include "chat_message.hpp"
#include "Shape.h"

using boost::asio::ip::tcp;
using namespace Patchwork;

//----------------------------------------------------------------------

typedef std::deque<chat_message> chat_message_queue;

//----------------------------------------------------------------------

class ClientConnection
{
public:
	virtual ~ClientConnection() {}
  virtual void deliver(const chat_message& msg) = 0;
  Image* img;
  int ID;
};

typedef std::shared_ptr<ClientConnection> ClientConnection_ptr;

//----------------------------------------------------------------------

class Room
{
public:
	void join(ClientConnection_ptr participant)
  {
    participants_.insert(participant);
  }

	void leave(ClientConnection_ptr participant)
  {
    participants_.erase(participant);
  }

  void deliver(const chat_message& msg)
  {
    for (auto participant: participants_)
      participant->deliver(msg);
  }

  void send_back_images()
  {
	  for (auto participant : participants_)
	  {
		  //get this participant image to string then send it
		  chat_message msg;
		  std::string s;
		  participant->img->serialize(s);
		  msg.body_length(s.length());
		  std::memcpy(msg.body(), s.c_str(), msg.body_length());
		  msg.encode_header();
		  participant->deliver(  msg );
	  }
  }

  void print_list()
  {
	  if (participants_.size())
	  {
		  for (auto participant : participants_)
		  {
			  std::cout << participant->ID << std::endl;
		  }
	  }
	  else
	  {
		  std::cout << "There are no clients connected to the server" << std::endl;
	  }
  }

  void annotate(int ID, std::string msg)
  {
	  for (auto participant : participants_)
	  {
		  if (participant->ID == ID)
		  {
			  participant->img->annotate(msg);
		  }
	  }
  }

  std::set<ClientConnection_ptr> participants()
  {
	  return participants_;
  }

private:
	std::set<ClientConnection_ptr> participants_;
  enum { max_recent_msgs = 100 };
  chat_message_queue recent_msgs_;
};

//----------------------------------------------------------------------

class Client
  : public ClientConnection,
    public std::enable_shared_from_this<Client>
{
public:
  Client(tcp::socket socket, Room& room, int ID)
    : socket_(std::move(socket)),
      room_(room)
  {
	  this->ID = ID;
	  img = new Image();
  }

  void start()
  {
    room_.join(shared_from_this());
    do_read_header();
  }

  void deliver(const chat_message& msg)
  {
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);
    if (!write_in_progress)
    {
      do_write();
    }
  }

private:
  void do_read_header()
  {
    auto self(shared_from_this());
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.data(), chat_message::header_length),
        [this, self](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec && read_msg_.decode_header())
          {
            do_read_body();
          }
          else
          {
            room_.leave(shared_from_this());
          }
        });
  }

  void do_read_body()
  {
    auto self(shared_from_this());
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
        [this, self](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
			std::string s = std::string(read_msg_.body());
			img->deserialize(s);
            do_read_header();
          }
          else
          {
            room_.leave(shared_from_this());
          }
        });
  }

  void do_write()
  {
    auto self(shared_from_this());
    boost::asio::async_write(socket_,
        boost::asio::buffer(write_msgs_.front().data(),
          write_msgs_.front().length()),
        [this, self](boost::system::error_code ec, std::size_t /*length*/)
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
            room_.leave(shared_from_this());
          }
        });
  }

  tcp::socket socket_;
  Room& room_;
  chat_message read_msg_;
  chat_message_queue write_msgs_;
};

//----------------------------------------------------------------------

class ServerIO
{
public:
  ServerIO(boost::asio::io_service& io_service,
      const tcp::endpoint& endpoint)
    : acceptor_(io_service, endpoint),
	socket_(io_service), ID(0)
  {
    do_accept();
  }

  void do_send()
  {
	  chat_message msg;
	  msg.body_length(std::strlen("GET"));
	  std::memcpy(msg.body(), "GET", msg.body_length());
	  msg.encode_header();
	  room_.deliver( msg );
  }

  void do_send_back()
  {
	  room_.send_back_images();
  }

  void do_print()
  {
	  room_.print_list();
  }

  void do_annotation(int ID, std::string msg)
  {
	  room_.annotate(ID, msg);
  }

  Room& room()
  {
	  return room_;
  }

private:
  void do_accept()
  {
    acceptor_.async_accept(socket_,
        [this](boost::system::error_code ec)
        {
          if (!ec)
          {
            std::make_shared<Client>(std::move(socket_), room_, ID++)->start();

			std::cout << "Nouvelle connection " << ID << std::endl;
          }

          do_accept();
        });
  }

  tcp::acceptor acceptor_;
  tcp::socket socket_;
  Room room_;
  int ID;
};

//----------------------------------------------------------------------

class Server
{
public:
	enum Commands { DISPLAY = 0, SEND, GET, PRINT, ANNOTATE, STATS, PATCHWORK, UNKNOWN };
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

	Server(boost::asio::io_service& service) : io_service(service)
	{
		tcp::endpoint endpoint(tcp::v4(), 8080);
		s = new ServerIO(io_service, std::move(endpoint));
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
					//TODO : make the user chose the image he wants to see
					int ID;
					std::string annotation;
					s->do_print();
					std::cout << "Choose an ID from the list :";
					std::cin >> ID;
					bool found_ID = false;
					for (auto participant : s->room().participants())
					{
						if (participant->ID == ID)
						{
							SDL_CreateWindowAndRenderer(800, 600, 0, &window, &renderer);
							while (1) {
								SDL_PollEvent(&event);
								if (event.type == SDL_QUIT) {
									break;
								}
								SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0x00);
								SDL_RenderClear(renderer);
								participant->img->display(renderer);
								SDL_RenderPresent(renderer);
							}
							SDL_DestroyWindow(window);
							found_ID = true;
							break;
						}
					}
					if (!found_ID)
					{
						std::cout << "ID : " << ID << " not found" << std::endl;
					}
				}break;

				case Commands::SEND:
				{
					s->do_send_back();
				}break;

				case Commands::GET:
				{
					s->do_send();
				}break;

				case Commands::PATCHWORK:
				{
					Image* Im = new Image();
					int last_x = 0;
					int origin_x = 0;
					for (auto participant : s->room().participants())
					{
						if (last_x == 0)
						{
							Im->add_component(participant->img);
							BoundingBox bb = participant->img->bounding_box();
							int w = bb.x_max - bb.x_min;
							last_x = last_x + (w / 2);
						}
						else
						{
							BoundingBox bb = participant->img->bounding_box();
							int w = bb.x_max - bb.x_min;
							origin_x = last_x + (w / 2);
							participant->img->origin(Vec2(origin_x, 0));
							Im->add_component(participant->img);
							last_x = last_x + w;							
						}						
					}
					SDL_CreateWindowAndRenderer(800, 600, 0, &window, &renderer);
					while (1) {
						SDL_PollEvent(&event);
						if (event.type == SDL_QUIT) {
							break;
						}
						SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0x00);
						SDL_RenderClear(renderer);
						Im->display(renderer);
						SDL_RenderPresent(renderer);
					}
					SDL_DestroyWindow(window);
					for (auto participant : s->room().participants())
					{
						participant->img->origin(Vec2(0, 0));
					}
				}break;

				case Commands::ANNOTATE:
				{
					//Make the user chose the image he wants to annotate
					// and then another cin to get the annotation
					s->do_print();
					int ID;
					std::string annotation;
					std::cout << "Choose an ID from the list :";
					std::cin >> ID;
					bool found_ID = false;
					for (auto participant : s->room().participants())
					{
						if (participant->ID == ID)
						{
							std::cout << "Enter your annotation :";
							std::getline(std::cin, annotation);
							std::getline(std::cin, annotation);
							s->do_annotation(ID, annotation);
							found_ID = true;
						}
					}
					if (!found_ID)
					{
						std::cout << "ID : " << ID << " not found" << std::endl;
					}
				}break;

				case Commands::STATS:
				{
					//NOTE(marc) : D'apr\E8s le standard, les types primitifs d'une map sont 
					// zero-initialis\E9, ont as pas besoin de la faire nous m\EAme
					std::map< Shape::Derivedtype, int > shapes_count;
					std::map< Color, int > color_count;
					for (auto participant : s->room().participants())
					{
						for (auto shape : participant->img->components())
						{
							if (shape->type() == Shape::IMAGE)
							{
								Image * im = static_cast<Image*>(shape);
								for (auto imageShape : im->components())
								{
									shapes_count[imageShape->type()]++;
									color_count[imageShape->color()]++;
								}
							}
							shapes_count[shape->type()]++;
							color_count[shape->color()]++;
						}
					}

					for (auto key_value : shapes_count)
					{
						switch (key_value.first)
						{
							case Shape::Derivedtype::CIRCLE:
							{
								std::cout << "Circle count : " << key_value.second << std::endl;
							}break;
							case Shape::Derivedtype::POLYGON:
							{
								std::cout << "Polygon count : " << key_value.second << std::endl;
							}break;
							case Shape::Derivedtype::LINE:
							{
								std::cout << "Line count : " << key_value.second << std::endl;
							}break;
							case Shape::Derivedtype::ELLIPSE:
							{
								std::cout << "Ellipse count : " << key_value.second << std::endl;
							}break;
						}
					}

					for (auto key_value : color_count)
					{
						std::cout << key_value.first << " : " << key_value.second << std::endl;
					}
				}break;

				case Commands::PRINT:
				{
					s->do_print();
				}break;

				default:
				{
					std::cout << "Unknow command" << std::endl;
				}
			}
		}
		t->join();
	}

	ServerIO* s;
	SDL_Event event;
	SDL_Window *window;
	SDL_Renderer *renderer;
	boost::asio::io_service& io_service;
	tcp::resolver* resolver;
	std::thread* t;
	std::vector<Image*> images;
};
const std::vector<std::string> Server::cmds = { "display", "send", "get", "print", "annotate", "stats", "patchwork" };


#if _WIN32_
int _tmain(int argc, _TCHAR* argv[])
#else
int main()
#endif
{
  try
  {

#if 0
    if (argc < 2)
    {
      std::cerr << "Usage: chat_server <port> [<port> ...]\n";
      //return 1;
    }
#endif
	boost::asio::io_service io_service;
	Server s(io_service);
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
