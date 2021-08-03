//
//	License type: BSD 3-Clause License
//	License copy: https://github.com/Telecominfraproject/wlan-cloud-ucentralgw/blob/master/LICENSE
//
//	Created by Stephane Bourque on 2021-03-04.
//	Arilia Wireless Inc.
//

#include "Poco/Array.h"
#include "Poco/JSON/Stringifier.h"

#include "RESTAPI_devices_handler.h"
#include "RESTAPI_protocol.h"
#include "StorageService.h"
#include "Utils.h"

#include "Utils.h"

namespace uCentral {
	void RESTAPI_devices_handler::handleRequest(Poco::Net::HTTPServerRequest &Request,
												Poco::Net::HTTPServerResponse &Response) {

		if (!ContinueProcessing(Request, Response))
			return;

		if (!IsAuthorized(Request, Response))
			return;

		try {
			if (Request.getMethod() == Poco::Net::HTTPRequest::HTTP_GET) {
				ParseParameters(Request);
				InitQueryBlock();
				auto serialOnly = GetBoolParameter(uCentral::RESTAPI::Protocol::SERIALONLY, false);
				auto countOnly = GetBoolParameter(uCentral::RESTAPI::Protocol::COUNTONLY, false);
				auto deviceWithStatus =
					GetBoolParameter(uCentral::RESTAPI::Protocol::DEVICEWITHSTATUS, false);

				Logger_.debug(Poco::format("DEVICES: from %Lu, limit of %Lu, filter='%s'.",
										   (uint64_t)QB_.Offset, (uint64_t)QB_.Limit, QB_.Filter));

				RESTAPIHandler::PrintBindings();

				Poco::JSON::Object RetObj;

				if (!QB_.Select.empty()) {

					Poco::JSON::Array Objects;
					std::vector<std::string> Numbers = uCentral::Utils::Split(QB_.Select);
					for (auto &i : Numbers) {
						GWObjects::Device D;
						if (Storage()->GetDevice(i, D)) {
							Poco::JSON::Object Obj;
							if (deviceWithStatus)
								D.to_json_with_status(Obj);
							else
								D.to_json(Obj);
							Objects.add(Obj);
						} else {
							Logger_.error(
								Poco::format("DEVICE(%s): device in select cannot be found.", i));
						}
					}

					if (deviceWithStatus)
						RetObj.set(uCentral::RESTAPI::Protocol::DEVICESWITHSTATUS, Objects);
					else
						RetObj.set(uCentral::RESTAPI::Protocol::DEVICES, Objects);

				} else if (countOnly == true) {
					uint64_t Count = 0;
					if (Storage()->GetDeviceCount(Count)) {
						RetObj.set(uCentral::RESTAPI::Protocol::COUNT, Count);
					}
				} else if (serialOnly) {
					std::vector<std::string> SerialNumbers;
					Storage()->GetDeviceSerialNumbers(QB_.Offset, QB_.Limit, SerialNumbers);
					Poco::JSON::Array Objects;
					for (const auto &i : SerialNumbers) {
						Objects.add(i);
					}
					RetObj.set(uCentral::RESTAPI::Protocol::SERIALNUMBERS, Objects);
				} else {
					std::vector<GWObjects::Device> Devices;
					Storage()->GetDevices(QB_.Offset, QB_.Limit, Devices);
					std::cout << "Offset:" << QB_.Offset << " Limit:" << QB_.Limit << " Device:" << Devices.size() << std::endl;
					Poco::JSON::Array Objects;
					for (const auto &i : Devices) {
						Poco::JSON::Object Obj;
						if (deviceWithStatus)
							i.to_json_with_status(Obj);
						else
							i.to_json(Obj);
						Objects.add(Obj);
					}
					if (deviceWithStatus)
						RetObj.set(uCentral::RESTAPI::Protocol::DEVICESWITHSTATUS, Objects);
					else
						RetObj.set(uCentral::RESTAPI::Protocol::DEVICES, Objects);
				}
				ReturnObject(Request, RetObj, Response);
				return;
			}
		} catch (const Poco::Exception &E) {
			Logger_.warning(
				Poco::format("%s: Failed with: %s", std::string(__func__), E.displayText()));
		}
		BadRequest(Request, Response);
	}
}