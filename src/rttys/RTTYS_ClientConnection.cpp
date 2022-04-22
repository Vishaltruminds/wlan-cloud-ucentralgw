//
// Created by stephane bourque on 2021-11-23.
//

#include "RTTYS_ClientConnection.h"
#include "rttys/RTTYS_device.h"
#include "rttys/RTTYS_server.h"

namespace OpenWifi {

	RTTYS_ClientConnection::RTTYS_ClientConnection(std::unique_ptr<Poco::Net::WebSocket> WS, std::string &Id,
							  Poco::Net::SocketReactor &Reactor)
			: 	WS_(std::move(WS)),
	  			Id_(std::move(Id)),
	  			SR_(Reactor),
				Logger_(RTTYS_server()->Logger()) {
        RTTYS_server()->Register(Id_, this);
		if(RTTYS_server()->CanConnect(Id_,this)) {
			Logger().information(fmt::format("{}: Starting WS connection, session: {}.", Id_, RTTYS_server()->DeviceSessionID(Id_)));
			SR_.addEventHandler(*WS_,
								Poco::NObserver<RTTYS_ClientConnection, Poco::Net::ReadableNotification>(
									*this, &RTTYS_ClientConnection::onSocketReadable));
			SR_.addEventHandler(*WS_,
								Poco::NObserver<RTTYS_ClientConnection, Poco::Net::ShutdownNotification>(
									*this, &RTTYS_ClientConnection::onSocketShutdown));

			auto DoLogin = [this]() -> void {
				int tries = 0 ;
				while(tries < 10) {
					if(RTTYS_server()->Login(this->Id_)) {
						Logger().information(fmt::format("{}: WS client connected to device, session: {}.", Id_, RTTYS_server()->DeviceSessionID(Id_)));
						this->Connected_=true;
						return;
					}
					std::this_thread::sleep_for(2000ms);
					tries++;
				}
				Logger().information(fmt::format("{}: WS client could not connect to device, session: {}.", Id_, RTTYS_server()->DeviceSessionID(Id_)));
				delete this;
			};

			std::thread CompleteConnection(DoLogin);
			CompleteConnection.detach();
		} else {
			Logger().information(fmt::format("{}: WS client cannot be connected.", Id_));
		    RTTYS_server()->DeRegister(Id_, this);
			delete this;
		}
	}

	RTTYS_ClientConnection::~RTTYS_ClientConnection() {
		Logger().information(fmt::format("{}: WS client disconnecting.", Id_));
		if(Connected_) {
			SR_.removeEventHandler(
				*WS_, Poco::NObserver<RTTYS_ClientConnection, Poco::Net::ReadableNotification>(
						 *this, &RTTYS_ClientConnection::onSocketReadable));
			SR_.removeEventHandler(
				*WS_, Poco::NObserver<RTTYS_ClientConnection, Poco::Net::ShutdownNotification>(
						 *this, &RTTYS_ClientConnection::onSocketShutdown));
		}
		WS_->close();
		if(Connected_) {
			RTTYS_server()->Logout(Id_);
			RTTYS_server()->DeRegister(Id_, this);
		}
	}

	void RTTYS_ClientConnection::Close() {

	}

	void RTTYS_ClientConnection::onSocketReadable([[maybe_unused]] const Poco::AutoPtr<Poco::Net::ReadableNotification> &pNf) {

		int flags;
		auto n = WS_->receiveFrame(Buffer, sizeof(Buffer), flags);
		auto Op = flags & Poco::Net::WebSocket::FRAME_OP_BITMASK;
		switch(Op) {
			case Poco::Net::WebSocket::FRAME_OP_PING: {
					WS_->sendFrame("", 0,(int)Poco::Net::WebSocket::FRAME_OP_PONG | (int)Poco::Net::WebSocket::FRAME_FLAG_FIN);
				}
				break;
			case Poco::Net::WebSocket::FRAME_OP_PONG: {
				}
				break;
			case Poco::Net::WebSocket::FRAME_OP_TEXT: {
					if (n == 0)
						return delete this;
					std::string s((char*)Buffer, n);
					std::cout << "TEXT: " << s << std::endl;
					auto Doc = nlohmann::json::parse(s);
					if(Doc.contains("type")) {
						auto Type = Doc["type"];
						if(Type == "winsize") {
							auto cols = Doc["cols"];
							auto rows = Doc["rows"];;
							auto Device = RTTYS_server()->GetDevice(Id_);
							if(Device==nullptr) {
								return;
							}
							Device->WindowSize(cols,rows);
						}
					}
				}
				break;
			case Poco::Net::WebSocket::FRAME_OP_BINARY: {
					if (n == 0)
						return delete this;
					auto Device = RTTYS_server()->GetDevice(Id_);
					if(Device==nullptr) {
						return;
					}
					Device->KeyStrokes(Buffer,n);
				}
				break;
			case Poco::Net::WebSocket::FRAME_OP_CLOSE: {
					WS_->shutdown();
					return delete this;
				}
				break;

			default:
			{

			}
		}
	}

	void RTTYS_ClientConnection::SendData( const u_char *Buf, size_t len ) {
		WS_->sendFrame(Buf, len, Poco::Net::WebSocket::FRAME_FLAG_FIN | Poco::Net::WebSocket::FRAME_OP_BINARY);
	}

	void RTTYS_ClientConnection::SendData( const std::string &s , bool login) {
		if(login) {
			RTTYS_server()->LoginDone(Id_);
		}
		WS_->sendFrame( s.c_str(), s.length());
	}

	void RTTYS_ClientConnection::onSocketShutdown([[maybe_unused]] const Poco::AutoPtr<Poco::Net::ShutdownNotification> &pNf) {
		delete this;
	}

}