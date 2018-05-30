﻿#include "BSQL.h"

namespace {
	std::unique_ptr<Library> library;
	std::string lastCreatedConnection, lastCreatedOperation, lastCreatedOperationConnectionId, lastRow, returnValueHolder;
}

BYOND_FUNC Initialize(const int argumentCount, const char* const* const args) noexcept {
	try {
		library = std::make_unique<Library>();
	}
	catch (std::bad_alloc&) {
		return "Out of memory!";
	}
	return nullptr;
}

BYOND_FUNC Shutdown(const int argumentCount, const char* const* const args) noexcept {
	if (!library)
		return "Library not initialized!";
	if (!lastCreatedConnection.empty())
		lastCreatedConnection = std::string();
	if (!lastCreatedOperation.empty())
		lastCreatedOperation = std::string();
	library.reset();
	return nullptr;
}

BYOND_FUNC GetError(const int argumentCount, const char* const* const args) noexcept {
	if (argumentCount != 2)
		return "Invalid arguments!";
	const auto& connectionIdentifier(args[0]), operationIdentifier(args[1]);
	if (!library)
		return "Library not initialized!";
	if (!connectionIdentifier)
		return "Invalid connection identifier!";
	if(!operationIdentifier)
		return "Invalid operation identifier!";
	try {
		auto connection(library->GetConnection(connectionIdentifier));
		if (!connection)
			return "Connection identifier does not exist!";
		auto operation(connection->GetOperation(operationIdentifier));
		if (!operation)
			return "Operation identifier does not exist!";
		if (!operation->IsComplete())
			return "Operation is not complete!";
		return operation->GetError().c_str();
	}
	catch (std::bad_alloc&) {
		return "Out of memory!";
	}
}

BYOND_FUNC CreateConnection(const int argumentCount, const char* const* const args) noexcept {
	if (argumentCount != 1)
		return "Invalid arguments!";
	if (!library)
		return "Library not initialized!";
	const auto& connectionType(args[0]);
	Connection::Type type;
	try {
		std::string conType(connectionType);
		if (conType == "MySql")
			type = Connection::Type::MySql;
		else if (conType == "SqlServer")
			type = Connection::Type::SqlServer;
		else
			return "Invalid connection type!";
	}
	catch (std::bad_alloc&) {
		return "Out of memory!";
	}

	if (!lastCreatedConnection.empty())
		//guess they didn't want it
		library->ReleaseConnection(lastCreatedConnection);

	auto result(library->CreateConnection(type));
	if (result.empty())
		return "Out of memory";

	lastCreatedConnection = std::move(result);
	return nullptr;
}

BYOND_FUNC GetConnection(const int argumentCount, const char* const* const args) noexcept {
	if (!library)
		return "Library not initialized!";
	if (lastCreatedConnection.empty())
		return nullptr;
	returnValueHolder = std::string();
	std::swap(returnValueHolder, lastCreatedConnection);
	return returnValueHolder.c_str();
}

BYOND_FUNC ReleaseConnection(const int argumentCount, const char* const* const args) noexcept {
	if (argumentCount != 1)
		return "Invalid arguments!";
	const auto& connectionIdentifier(args[0]);
	if (!connectionIdentifier)
		return "Invalid connection identifier!";
	if (!library)
		return "Library not initialized!";
	try {
		if (!library->ReleaseConnection(connectionIdentifier))
			return "Connection identifier does not exist!";
	}
	catch (std::bad_alloc&) {
		return "Out of memory!";
	}
	return nullptr;
}

BYOND_FUNC GetOperation(const int argumentCount, const char* const* const args) noexcept {
	if (!library)
		return "Library not initialized!";
	if (lastCreatedOperation.empty())
		return nullptr;
	returnValueHolder = std::string();
	std::swap(returnValueHolder, lastCreatedOperation);
	return returnValueHolder.c_str();
}

BYOND_FUNC ReleaseOperation(const int argumentCount, const char* const* const args) noexcept {
	if (lastCreatedOperation.empty())
		return nullptr;
	returnValueHolder = std::string();
	std::swap(returnValueHolder, lastCreatedOperation);
	return returnValueHolder.c_str();
}

BYOND_FUNC OpenConnection(const int argumentCount, const char* const* const args) noexcept {
	if (argumentCount != 5)
		return "Invalid arguments!";
	const auto& connectionIdentifier(args[0]), ipaddress(args[1]), port(args[2]), username(args[3]), password(args[4]);

	if (!connectionIdentifier)
		return "Invalid connection identifier!";
	if (!ipaddress)
		return "Invalid ip address!";
	if (!port)
		return "Invalid port!";
	if (!password)
		return "Invalid password!";
	unsigned short realPort;
	bool outOfRange;
	try {
		auto asInt(std::stoi(port));
		outOfRange = asInt < 0 || asInt > std::numeric_limits<unsigned short>::max();
		if (!outOfRange)
			realPort = asInt;
	}
	catch (std::invalid_argument&) {
		return "Port is not a number!";
	}
	catch (std::out_of_range&) {
		outOfRange = true;
	}
	if (outOfRange)
		return "Port is out of acceptable range!";

	if (!library)
		return "Library not initialized!";
	try {
		//clear the cache
		GetOperation(0, nullptr);
		lastCreatedOperationConnectionId = connectionIdentifier;
		auto connection(library->GetConnection(lastCreatedOperationConnectionId));
		if (!connection)
			return "Connection identifier does not exist!";
		lastCreatedOperation = connection->Connect(ipaddress, realPort, username, password);
		return nullptr;
	}
	catch (std::bad_alloc&) {
		return "Out of memory!";
	}
}

BYOND_FUNC NewQuery(const int argumentCount, const char* const* const args) noexcept {
	if (argumentCount != 2)
		return "Invalid arguments!";
	const auto& connectionIdentifier(args[0]), queryText(args[1]);
	if (!connectionIdentifier)
		return "Invalid connection identifier!";
	if (!queryText)
		return "Invalid query text!";
	if (!library)
		return "Library not initialized!";
	try {
		//clear the cache
		GetOperation(0, nullptr);
		auto connection(library->GetConnection(lastCreatedOperationConnectionId));
		if (!connection)
			return "Connection identifier does not exist!";
		lastCreatedOperation = connection->CreateQuery(queryText);
		return nullptr;
	}
	catch (std::bad_alloc&) {
		return "Out of memory!";
	}
}

const char* TryLoadQuery(const int argumentCount, const char* const* const args, Query** query) noexcept {
	if (argumentCount != 2)
		return "Invalid arguments!";
	const auto& connectionIdentifier(args[0]), operationIdentifier(args[1]);
	if (!connectionIdentifier)
		return "Invalid connection identifier!";
	if (!operationIdentifier)
		return "Invalid operation identifier!";

	try {
		auto connection(library->GetConnection(lastCreatedOperationConnectionId));
		if (!connection)
			return "Connection identifier does not exist!";
		auto operation(connection->GetOperation(operationIdentifier));
		if (!operation)
			return "Operation identifier does not exist!";
		if (!operation->IsQuery())
			return "Operation is not a query!";
		*query = static_cast<Query*>(operation);
		return nullptr;
	}
	catch (std::bad_alloc&) {
		return "Out of memory!";
	}
}

BYOND_FUNC GetRow(const int argumentCount, const char* const* const args) noexcept {
	if (!library)
		return "Library not initialized!";
	if (lastRow.empty())
		return nullptr;
	returnValueHolder = std::string();
	std::swap(returnValueHolder, lastRow);
	return returnValueHolder.c_str();
}

BYOND_FUNC ReadyRow(const int argumentCount, const char* const* const args) noexcept {
	Query* query;
	auto res(TryLoadQuery(argumentCount, args, &query));
	if (res != nullptr)
		return res;
	if (query->IsComplete())
		try {
			lastRow = query->CurrentRow();
			return "DONE";
		}
		catch (std::bad_alloc&) {
			return "Out of memory!";
		}
	return "NOTDONE";
}

BYOND_FUNC BeginFetchNextRow(const int argumentCount, const char* const* const args) noexcept {
	Query* query;
	auto res(TryLoadQuery(argumentCount, args, &query));
	if (res != nullptr)
		return res;
	if (!query->BeginGetNextRow())
		return "Query in progress!";
	return nullptr;
}