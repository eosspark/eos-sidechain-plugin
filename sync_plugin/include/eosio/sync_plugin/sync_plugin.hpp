#pragma once
#include <appbase/application.hpp>
#include <eosio/chain/action.hpp>

#include <eosio/chain/controller.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/client_plugin/client_plugin.hpp>
namespace eosio {
	using boost::signals2::signal;

	class sync_plugin : public appbase::plugin<sync_plugin> {
	public:
		APPBASE_PLUGIN_REQUIRES((chain_plugin))

		sync_plugin();

		virtual ~sync_plugin();

		virtual void set_program_options(options_description &cli, options_description &cfg) override;

		void plugin_initialize(const variables_map& options);
		void plugin_startup();
		void plugin_shutdown();

//		signal<void(const chain::producer_confirmation&)> confirmed_block;
	private:
		std::shared_ptr<class sync_plugin_impl> my;
	};

//	class read_only {
//		 shared_ptr<class sync_plugin_impl> sync;
//
//		 public:
//		 	read_only(shared_ptr<class sync_plugin_impl> sync) : sync(sync) {}
//
//		 	struct get_executed_transaction_result {
//
//		 	};
//
//		 get_executed_transaction_result get_executed_transaction() const;
//
//	};



} /// namespace eosio
