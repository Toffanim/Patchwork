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
#include <tchar.h>
#include <boost/asio.hpp>
#include "chat_message.hpp"
#include "Shape.h"

using boost::asio::ip::tcp;
using namespace Patchwork;

//----------------------------------------------------------------------

typedef std::deque<chat_message> chat_message_queue;

//----------------------------------------------------------------------

class chat_participant
{
public:
  virtual ~chat_participant() {}
  virtual void deliver(const chat_message& msg) = 0;
  Image img;
  int ID;
};

typedef std::shared_ptr<chat_participant> chat_participant_ptr;

//----------------------------------------------------------------------

class chat_room
{
public:
  void join(chat_participant_ptr participant)
  {
    participants_.insert(participant);
  }

  void leave(chat_participant_ptr participant)
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
		  participant->img.serialize(s);
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
			  participant->img.annotate(msg);
		  }
	  }
  }

  std::set<chat_participant_ptr> participants()
  {
	  return participants_;
  }

private:
  std::set<chat_participant_ptr> participants_;
  enum { max_recent_msgs = 100 };
  chat_message_queue recent_msgs_;
};

//----------------------------------------------------------------------

class chat_session
  : public chat_participant,
    public std::enable_shared_from_this<chat_session>
{
public:
  chat_session(tcp::socket socket, chat_room& room, int ID)
    : socket_(std::move(socket)),
      room_(room)
  {
	  this->ID = ID;
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
			  std::cout << "Received msg : header read" << std::endl;
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
			std::cout << "Received msg : body read" << read_msg_.body() << std::endl;
			std::string s = std::string(read_msg_.body());
			img.deserialize(s);
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
  chat_room& room_;
  chat_message read_msg_;
  chat_message_queue write_msgs_;
};

//----------------------------------------------------------------------

class Server
{
public:
  Server(boost::asio::io_service& io_service,
      const tcp::endpoint& endpoint)
    : acceptor_(io_service, endpoint),
	socket_(io_service), ID(0)
  {
    do_accept();
  }

  void do_send()
  {
	  std::cout << "Do send" << std::endl;
	  chat_message msg;
	  msg.body_length(std::strlen("TEST"));
	  std::memcpy(msg.body(), "TEST", msg.body_length());
	  msg.encode_header();
	  room_.deliver( msg );
  }

  void do_send_back()
  {
	  std::cout << "Do send back" << std::endl;
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

  chat_room& room()
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
            std::make_shared<chat_session>(std::move(socket_), room_, ID++)->start();

			std::cout << "Nouvelle connection " << ID << std::endl;
          }

          do_accept();
        });
  }

  tcp::acceptor acceptor_;
  tcp::socket socket_;
  chat_room room_;
  int ID;
};

//----------------------------------------------------------------------

int _tmain(int argc, _TCHAR* argv[])
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

	SDL_Init(SDL_INIT_VIDEO);
	SDL_Event event;
	SDL_Window *window;
	SDL_Renderer *renderer;
	int ID = 0;

    boost::asio::io_service io_service;
	//tcp::endpoint endpoint(tcp::v4(), std::atoi(argv[i]));
	tcp::endpoint endpoint(tcp::v4(), 8080);
	Server s(io_service, endpoint);
	std::thread t([&io_service](){ io_service.run(); });

	char line[chat_message::max_body_length + 1];
	std::string line_str;
	while (std::cin.getline(line, chat_message::max_body_length + 1))
	{
		line_str = std::string(line);
		if (line_str == "display")
		{
			//TODO : make the user chose the image he wants to see
			int ID;
			std::string annotation;
			s.do_print();
			std::cout << "Choose an ID from the list :";
			std::cin >> ID;
			bool found_ID = false;
			for (auto participant : s.room().participants())
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
						participant->img.display(renderer);
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
		}

		if (line_str == "send_back")
		{
			s.do_send_back();
		}

		if (line_str == "get_drawings")
		{
			s.do_send();
		}

		if (line_str == "annotate")
		{
			//Make the user chose the image he wants to annotate
			// and then another cin to get the annotation
			s.do_print();
			int ID;
			std::string annotation;
			std::cout << "Choose an ID from the list :";
			std::cin >> ID;
			bool found_ID = false;
			for (auto participant : s.room().participants())
			{
				if (participant->ID == ID)
				{
					std::cout << "Enter your annotation :";
					std::getline(std::cin, annotation);
					std::getline(std::cin, annotation);
					s.do_annotation(ID, annotation);
					found_ID = true;
				}
			}
			if (!found_ID)
			{
				std::cout << "ID : " << ID << " not found" << std::endl;
			}
		}

		if (line_str == "print_list")
		{
			s.do_print();
		}

		if (line_str == "make_stats")
		{
			//NOTe(marc) : D'après le standard, les types primitifs d'une map sont 
			// zero-initialisé, ont as pas besoin de la faire nous même
			std::map< Shape::Derivedtype, int > shapes_count;
			std::map< Color, int > color_count;
			for (auto participant : s.room().participants())
			{
				for (auto shape : participant->img.components())
				{
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
		}
	}


	t.join();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
