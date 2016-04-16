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
#if _WIN32
#include <tchar.h>
#endif
#include <boost/asio.hpp>
#include "Message.hpp"
#include "Shape.h"

using boost::asio::ip::tcp;
using namespace Patchwork;

/*! \file Server.cpp
\brief File containing the server part of the application

*/

//----------------------------------------------------------------------

typedef std::deque<Message> Message_queue;

//----------------------------------------------------------------------
/*!
Abstract class for handling Client.
A client has an image and a unique ID associated to it.
*/
class ClientConnection
{
public:
	virtual ~ClientConnection() {}
  virtual void deliver(const Message& msg) = 0;
  Image* img; /*!< The image linked to the client */
  int ID; /*!< unique ID identifying the client */
};

typedef std::shared_ptr<ClientConnection> ClientConnection_ptr;

//----------------------------------------------------------------------

/*!
The room is responsible for maintening an updated list of client and 
*/
class Room
{
public:
	/*!
	Add participant to the room
	*/
   void join(ClientConnection_ptr participant)
  {
    participants_.insert(participant);
  }
   /*!
   Delete participant from the room
   */
	void leave(ClientConnection_ptr participant)
  {
    participants_.erase(participant);
  }
	/*!
	Getter of participant list of the room
	*/
  std::set<ClientConnection_ptr> participants()
  {
	  return participants_;
  }

private:
	std::set<ClientConnection_ptr> participants_;  /*!< List of participants */
};

//----------------------------------------------------------------------


/*!
The Client class handle the client, joining the room and being the one doing asynchronous operations.
*/
class Client
  : public ClientConnection,
    public std::enable_shared_from_this<Client>
{
public:
	/*!
	Create a client with an associated socket, room, image and ID
	*/
  Client(tcp::socket socket, Room& room, int ID)
    : socket_(std::move(socket)),
      room_(room)
  {
	  this->ID = ID;
	  img = new Image();
  }
  /*!
  Join the room and try to read from the socket
  */
  void start()
  {
    room_.join(shared_from_this());
    do_read_header();
  }
  /*!
  Write messages
  */
  void deliver(const Message& msg)
  {
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);
    if (!write_in_progress)
    {
      do_write();
    }
  }

private:
	/*!
	Read from the socket into a buffer and analyze our message header, then ask to read the message's body
	*/
  void do_read_header()
  {
    auto self(shared_from_this());
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.data(), Message::header_length),
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
  /*!
  Read from the socket into a buffer and analyze our message body, then start again to read from the socket is some reads are needed to be done (due to asynchronous design)
  If it has something to read, it's an image so it deserialize it
  */
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
  /*!
  Write to the socket, then ask to write again if some writes are needed to be done (due to asychronous design)
  */
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

  tcp::socket socket_; /*!< boost:asio TCP socket */
  Room& room_; /*!< The room in which the client is connected */
  Message read_msg_; /*!< The message being read */
  Message_queue write_msgs_; /*!< A list of message de send (due to asynchronous design) */
};

//----------------------------------------------------------------------

/*!
Class that handle the input and output of the server (basically reading and writing to the socket)
*/
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
  /*!
  Create a "GET" message and send it to all the client connected to the room
  */
  bool do_send()
  {
	  if (room_.participants().size())
	  {
		  Message msg;
		  msg.body_length(std::strlen("GET"));
		  std::memcpy(msg.body(), "GET", msg.body_length());
		  msg.encode_header();
		  for (auto participant : room_.participants())
			  participant->deliver(msg);
		  return true;
	  }
	  else
	  {
		  std::cout << "There are no clients connected to the server" << std::endl;
		  return false;
	  }
  }
  /*!
  Send back all the drawings to all the client connected to the room
  */
  bool do_send_back()
  {
	  if (room_.participants().size())
	  {
		  for (auto participant : room_.participants())
		  {
			  //get this participant image to string then send it
			  Message msg;
			  std::string s;
			  participant->img->serialize(s);
			  msg.body_length(s.length());
			  std::memcpy(msg.body(), s.c_str(), msg.body_length());
			  msg.encode_header();
			  participant->deliver(msg);
		  }
		  return true;
	  }
	  else
	  {
		  std::cout << "There are no clients connected to the server" << std::endl;
		  return false;
	  }
  }
  /*!
  Print all the client connected to the room
  */
  bool do_print()
  {
	  if (room_.participants().size())
	  {
		  std::cout << "Client ID : " << std::endl;
		  for (auto participant : room_.participants())
		  {
			   std::cout << participant->ID << std::endl;
		  }
		  return true;
	  }
	  else
	  {
		  std::cout << "There are no clients connected to the server" << std::endl;
		  return false;
	  }
  }
  /*!
  Give the image associated the the Client ID the annotation contained in msg
  */
  void do_annotation(int ID, std::string msg)
  {
	  for (auto participant : room_.participants())
	  {
		  if (participant->ID == ID)
		  {
			  participant->img->annotate(msg);
		  }
	  }
  }
  /*!
  Getter for the room
  */
  Room& room()
  {
	  return room_;
  }

private:
	/*!
	Accept all incoming connection, recursively (due to asynchronous design)
	*/
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

  tcp::acceptor acceptor_; /*!< boost::asio acceptor (the core object of a server) that can accept connections */
  tcp::socket socket_; /*!< boost::asio TCP Socket */
  Room room_; /*!< A room allocated to the server */
  int ID; /*!< An ID which will be incremented at each connections */
};

//----------------------------------------------------------------------

/*!
Class that handle the Server's input commands, basically polling commands from the console and reacting to it
*/
class Server
{
public:
	enum Commands { DISPLAY = 0, SEND, GET, PRINT, ANNOTATE, STATS, PATCHWORK, HELP, QUIT, UNKNOWN }; /*!< Enums of available commands */
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
	Class that creates the Server and poll user input to execute commands
	\param service boost::asio io_service
	*/
	Server(boost::asio::io_service& service) : io_service(service)
	{
		//Init socket
		tcp::endpoint endpoint(tcp::v4(), 8080);
		s = new ServerIO(io_service, std::move(endpoint));
		t = new std::thread([&](){ io_service.run(); });
		SDL_Init(SDL_INIT_VIDEO);
		start_polling();
	};

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

			switch (CmdStringToEnum(cmd))
			{
				case Commands::DISPLAY:
				{
					int ID;
					std::string annotation;
					if (s->do_print())
					{
						std::cout << "Choose an ID from the list :";
						try
						{
							std::cin >> ID;
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
					}
				}break;

				case Commands::SEND:
				{
					if ( s->do_send_back())
					    std::cout << "Images sent" << std::endl;
				}break;

				case Commands::GET:
				{
					if ( s->do_send() ) 
					    std::cout << "Get images on progress | use \"print\" to check when it is done" << std::endl;
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
					if (s->do_print())
					{
						int ID;
						std::string annotation;
						std::cout << "Choose an ID from the list :";
						try
						{
							std::cin >> ID;
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
								std::cout << "Annotation entered" << std::endl;
								break;
							}
						}
						if (!found_ID)
						{
							std::cout << "ID : " << ID << " not found" << std::endl;
						}
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

				case Commands::HELP:
				{
					print_commands();
					std::cout << std::endl;
				}break;

				case Commands::QUIT:
				{
					quit = true;
				}break;

				default:
				{
					std::cout << "Unknow command" << std::endl;
				}
			}


			if (quit)
				break;

			std::cin.clear();
			std::cin.ignore(100000, '\n');
			std::cout << std::endl << "Command : ";

		}
		io_service.stop();
		t->join();
	}

	ServerIO* s; /*!< A list of message de send (due to asynchronous design) */
	SDL_Event event; /*!< SDL Event so we can know when to close the window */
	SDL_Window *window; /*!< SDL window to display to */
	SDL_Renderer *renderer; /*!< SDL renderer to draw components to */
	boost::asio::io_service& io_service;  /*!< boost::asio io_service */
	tcp::resolver* resolver; /*!< boost::asio TCP resolver */
	std::thread* t;  /*!< Thread polling Input/Output event from io_service */
};
const std::vector<std::string> Server::cmds = { "display", "send", "get", "print", "annotate", "stats", "patchwork", "help" , "quit"};


#if _WIN32
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
