//
// Created by stephane bourque on 2021-03-04.
//

#ifndef UCENTRAL_UAUTHSERVICE_H
#define UCENTRAL_UAUTHSERVICE_H

#include "SubSystemServer.h"

#include "Poco/JSON/Object.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/JWT/Signer.h"

namespace uCentral::Auth {

	enum ACCESS_TYPE {
		USERNAME,
		SERVER,
		CUSTOM
	};

	ACCESS_TYPE IntToAccessType(int C);
	int AccessTypeToInt(ACCESS_TYPE T);

    struct AclTemplate {
        bool Read_ = true ;
        bool ReadWrite_ = true ;
        bool ReadWriteCreate_ = true ;
        bool Delete_ = true ;
        bool PortalLogin_ = true ;
        void to_JSON(Poco::JSON::Object &Obj) const ;
    };

    struct WebToken {
        std::string access_token_;
        std::string refresh_token_;
        std::string id_token_;
        std::string token_type_;
        std::string username_;
        unsigned int expires_in_;
        unsigned int idle_timeout_;
        AclTemplate acl_template_;
        uint64_t    created_;
        void to_JSON(Poco::JSON::Object &Obj) const ;
    };

    int Start();
    void Stop();
    bool IsAuthorized(Poco::Net::HTTPServerRequest & Request,std::string &SessionToken, struct WebToken & UserInfo );
    bool Authorize( const std::string & UserName, const std::string & Password, WebToken & ResultToken );
    void Logout(const std::string &token);

    class Service : public SubSystemServer {
    public:

        Service() noexcept;

        friend int Start();
        friend void Stop();

        static Service *instance() {
            if (instance_ == nullptr) {
                instance_ = new Service;
            }
            return instance_;
        }

        friend bool IsAuthorized(Poco::Net::HTTPServerRequest & Request,std::string &SessionToken, struct WebToken & UserInfo );
        friend bool Authorize( const std::string & UserName, const std::string & Password, WebToken & ResultToken );
        [[nodiscard]] std::string GenerateToken(const std::string & UserName, ACCESS_TYPE Type, int NumberOfDays);
		[[nodiscard]] bool ValidateToken(const std::string & Token, std::string & SessionToken, struct WebToken & UserInfo  );
        friend void Logout(const std::string &token);

    private:
        int Start() override;
        void Stop() override;
        bool IsAuthorized(Poco::Net::HTTPServerRequest & Request,std::string &SessionToken, struct WebToken & UserInfo );
        void CreateToken(const std::string & UserName, WebToken & ResultToken);
        bool Authorize( const std::string & UserName, const std::string & Password, WebToken & ResultToken );
        void Logout(const std::string &token);

        static Service *instance_;
        std::map<std::string,WebToken>   Tokens_;
        bool    			Secure_ = false ;
        std::string     	DefaultUserName_,
                        	DefaultPassword_;
        std::string     	Mechanism_;
        bool            	AutoProvisioning_ = false ;
		Poco::JWT::Signer	Signer_;
    };

}; // end of namespace

#endif //UCENTRAL_UAUTHSERVICE_H
