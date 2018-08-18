//
// Created by 超超 on 2018/8/17.
//
#include <eosio/sync_api_plugin/sync_api_plugin.hpp>
#include <eosio/chain/exceptions.hpp>

#include <fc/io/json.hpp>

namespace eosio {
using namespace eosio;

static appbase::abstract_plugin& _sync_api_plugin = app().register_plugin<sync_api_plugin>();

sync_api_plugin::sync_api_plugin() {}
sync_api_plugin::~sync_api_plugin() {}

void sync_api_plugin::set_program_options(boost::program_options::options_description &,
														boost::program_options::options_description &) {}
void sync_api_plugin::plugin_initialize(const boost::program_options::variables_map &) {}

#define CALL(api_name, api_handle, api_namespace, call_name) \
{std::string("/v1/" #api_name "/" #call_name), \
   [this, api_handle](string, string body, url_response_callback cb) mutable { \
          try { \
             if (body.empty()) body = "{}"; \
             auto result = api_handle.call_name(); \
             cb(200, fc::json::to_string(result)); \
          } catch (...) { \
             http_plugin::handle_exception(#api_name, #call_name, body, cb); \
          } \
       }}

#define CHAIN_RO_CALL(call_name) CALL(sync, ro_api, sync_apis::read_only, call_name)

void sync_api_plugin::plugin_startup() {
   ilog( "starting sync_api_plugin" );
   auto ro_api = app().get_plugin<sync_plugin>().get_read_only_api();
   app().get_plugin<http_plugin>().add_api({
      CHAIN_RO_CALL(get_executed_transaction),
      CHAIN_RO_CALL(get_success_transaction),
      CHAIN_RO_CALL(get_failure_transaction)
});
}

void sync_api_plugin::plugin_shutdown() {}

}
