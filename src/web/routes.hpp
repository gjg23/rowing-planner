// src/web/routes.hpp
#pragma once

namespace httplib { class Server; }
class Database;
struct Config;

void register_routes(httplib::Server& svr, Database& db, const Config& cfg);