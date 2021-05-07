//
// Created by stephane bourque on 2021-02-15.
//

#ifndef UCENTRAL_UCENTRAL_H
#define UCENTRAL_UCENTRAL_H

#include <iostream>
#include <cstdlib>

#include "Poco/Util/Application.h"
#include "Poco/Util/ServerApplication.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/UUIDGenerator.h"
#include "Poco/ErrorHandler.h"
#include "Poco/Crypto/RSAKey.h"

using Poco::Util::ServerApplication;

namespace uCentral {

	class MyErrorHandler : public Poco::ErrorHandler {
    public:
        void exception(const Poco::Exception & E) override;
        void exception(const std::exception & E) override;
        void exception() override;
    private:

    };

    class Daemon : public ServerApplication {

    public:
		int main(const ArgVec &args) override;
        void initialize(Application &self) override;
        void uninitialize() override;
        void reinitialize(Application &self) override;
        void defineOptions(Poco::Util::OptionSet &options) override;
        void handleHelp(const std::string &name, const std::string &value);
		void handleVersion(const std::string &name, const std::string &value);
        void handleDebug(const std::string &name, const std::string &value);
        void handleLogs(const std::string &name, const std::string &value);
        void handleConfig(const std::string &name, const std::string &value);
        void displayHelp();

        std::string CreateUUID();
        std::string IdentifyDevice(const std::string & Id ) const;
        bool AutoProvisioning() const { return AutoProvisioning_ ; }
        bool Debug() const { return DebugMode_; }
        uint64_t ID() const { return ID_; }
		static bool SetSubsystemLogLevel(const std::string & SubSystem, const std::string & Level);
		static std::string Version();
		const Poco::SharedPtr<Poco::Crypto::RSAKey> & Key() { return AppKey_; }
		void Exit(int Reason);

    private:
        bool                        HelpRequested_ = false;
        bool                        AutoProvisioning_ = false;
        std::map<std::string,std::vector<std::string>>    DeviceTypeIdentifications_;
        std::string                 ConfigFileName_;
        std::string                 LogDir_;
        bool                        DebugMode_ = false;
        uint64_t                    ID_ = 1;
        Poco::UUIDGenerator         UUIDGenerator_;
        MyErrorHandler              AppErrorHandler_;
		Poco::SharedPtr<Poco::Crypto::RSAKey>	AppKey_ = nullptr;
    };

    namespace ServiceConfig {
        uint64_t GetInt(const std::string &Key,uint64_t Default);
        uint64_t GetInt(const std::string &Key);
        std::string GetString(const std::string &Key,const std::string & Default);
        std::string GetString(const std::string &Key);
        uint64_t GetBool(const std::string &Key,bool Default);
        uint64_t GetBool(const std::string &Key);
    }

	Daemon * instance();
}

#endif //UCENTRAL_UCENTRAL_H
