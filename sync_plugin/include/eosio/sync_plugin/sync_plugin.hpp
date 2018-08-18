#pragma once
#include <appbase/application.hpp>
#include <eosio/chain/action.hpp>

#include <eosio/chain/controller.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/client_plugin/client_plugin.hpp>
namespace eosio {

	 using boost::signals2::signal;

	 typedef std::shared_ptr<const class sync_plugin_impl> sync_const_ptr;

	 namespace sync_apis {
		  class read_only {
				sync_const_ptr sync;

		  public:
				read_only(shared_ptr<class sync_plugin_impl> sync) : sync(sync) {}

				struct get_executed_transaction_result {
					 transaction_id_type trx_id;
					 account_name from;
					 account_name to;
					 asset quantity;
					 time_point timestamp;
					 uint8_t count;

					 get_executed_transaction_result(transaction_id_type trx_id, account_name from, account_name to,
																asset quantity, time_point timestamp, uint8_t count)
								: trx_id(trx_id), from(from), to(to), quantity(quantity), timestamp(timestamp), count(count) {}

					 get_executed_transaction_result(transaction_id_type trx_id, account_name from, account_name to,
																asset quantity, time_point timestamp)
								: trx_id(trx_id), from(from), to(to), quantity(quantity), timestamp(timestamp) {}
				};

				vector<get_executed_transaction_result> get_executed_transaction() const;
				vector<get_executed_transaction_result> get_success_transaction() const;
				vector<get_executed_transaction_result> get_failure_transaction() const;

		  };
	 } /// namespace sync_apis

	 class sync_plugin : public appbase::plugin<sync_plugin> {
	 public:
		  APPBASE_PLUGIN_REQUIRES((chain_plugin))

		  sync_plugin();

		  virtual ~sync_plugin();

		  virtual void set_program_options(options_description &cli, options_description &cfg) override;

		  void plugin_initialize(const variables_map& options);
		  void plugin_startup();
		  void plugin_shutdown();

		  sync_apis::read_only  get_read_only_api() const { return sync_apis::read_only(my); }

	 private:
		  std::shared_ptr<class sync_plugin_impl> my;
	 };



} /// namespace eosio

FC_REFLECT( eosio::sync_apis::read_only::get_executed_transaction_result, (trx_id)(from)(to)(quantity)(timestamp)(count) )