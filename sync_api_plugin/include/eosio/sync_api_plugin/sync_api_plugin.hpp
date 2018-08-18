//
// Created by 超超 on 2018/8/17.
//

#pragma once
#include <eosio/sync_plugin/sync_plugin.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/http_plugin/http_plugin.hpp>

#include <appbase/application.hpp>

namespace eosio {

	 using namespace appbase;

	 class sync_api_plugin : public plugin<sync_api_plugin> {
	 public:
		  APPBASE_PLUGIN_REQUIRES((sync_plugin)(chain_plugin)(http_plugin))

		  sync_api_plugin();
		  virtual ~sync_api_plugin();

		  virtual void set_program_options(options_description&, options_description&) override;

		  void plugin_initialize(const variables_map&);
		  void plugin_startup();
		  void plugin_shutdown();

	 private:
	 };


}