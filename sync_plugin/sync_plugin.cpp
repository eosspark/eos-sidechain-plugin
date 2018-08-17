//
// Created by yc、sf on 2018/8/1.
//

#include <eosio/sync_plugin/sync_plugin.hpp>

namespace eosio {

	using namespace chain;
	using action_data = vector<char>;

	static appbase::abstract_plugin &_sync_plugin = app().register_plugin<sync_plugin>();

   struct cactus_transfer {
		account_name from;
		account_name to;
		asset quantity;
  	};

  	struct cactus_msigtrans {
		account_name user;
		transaction_id_type trx_id;
		account_name from;
		account_name to;
		asset quantity;
  	};

	struct by_block_num;
	struct by_timestamp;
	struct by_trx_id;

 	struct transaction_reversible_object
			: public chainbase::object<transaction_reversible_object_type, transaction_reversible_object> { \
			OBJECT_CTOR(transaction_reversible_object)
			id_type id;
			uint32_t block_num;
			transaction_id_type trx_id;
			action_data data;
	};

	using transaction_reversible_multi_index = chainbase::shared_multi_index_container<
		transaction_reversible_object,
		indexed_by<
				ordered_unique<tag<by_id>, member<transaction_reversible_object, transaction_reversible_object::id_type, &transaction_reversible_object::id>>,
				ordered_unique<tag<by_trx_id>, member<transaction_reversible_object, transaction_id_type, &transaction_reversible_object::trx_id>>,
				ordered_non_unique<tag<by_block_num>, member<transaction_reversible_object, uint32_t, &transaction_reversible_object::block_num>>
		>
	>;

   struct transaction_executed_object
			: public chainbase::object<transaction_executed_object_type, transaction_executed_object> {
			OBJECT_CTOR(transaction_executed_object)
			id_type id;
			transaction_id_type trx_id;
			action_data data;
			time_point timestamp;
			uint32_t count;
	};

	using transaction_executed_multi_index = chainbase::shared_multi_index_container<
		transaction_executed_object,
		indexed_by<
				ordered_unique<tag<by_id>, member<transaction_executed_object, transaction_executed_object::id_type, &transaction_executed_object::id>>,
				ordered_unique<tag<by_trx_id>, member<transaction_executed_object, transaction_id_type, &transaction_executed_object::trx_id>>,
				ordered_non_unique<tag<by_timestamp>, member<transaction_executed_object, time_point, &transaction_executed_object::timestamp>>
		>
	>;

	struct transaction_success_object
			  : public chainbase::object<transaction_success_object_type, transaction_success_object> {
		 OBJECT_CTOR(transaction_success_object)
		 id_type id;
		 transaction_id_type trx_id;
		 action_data data;
		 time_point timestamp;
	};

	 using transaction_success_multi_index = chainbase::shared_multi_index_container<
		transaction_success_object,
		indexed_by<
				  ordered_unique<tag<by_id>, member<transaction_success_object, transaction_success_object::id_type, &transaction_success_object::id>>,
				  ordered_unique<tag<by_trx_id>, member<transaction_success_object, transaction_id_type, &transaction_success_object::trx_id>>,
				  ordered_non_unique<tag<by_timestamp>, member<transaction_success_object, time_point, &transaction_success_object::timestamp>>
		>
	 >;
}
FC_REFLECT( eosio::cactus_transfer, (from)(to)(quantity))
FC_REFLECT( eosio::cactus_msigtrans, (user)(trx_id)(from)(to)(quantity))

CHAINBASE_SET_INDEX_TYPE(eosio::transaction_reversible_object, eosio::transaction_reversible_multi_index)
CHAINBASE_SET_INDEX_TYPE(eosio::transaction_executed_object, eosio::transaction_executed_multi_index)
CHAINBASE_SET_INDEX_TYPE(eosio::transaction_success_object, eosio::transaction_success_multi_index)

#ifndef DATA_FORMAT
#define DATA_FORMAT(user, trx_id, from, to, quantity) "[\""+user+"\", \""+trx_id+"\", \""+from+"\", \""+to+"\", \""+quantity+"\"]"
#endif


namespace eosio {

	class sync_plugin_impl {
	public:
		chain_plugin*     chain_plug = nullptr;
		fc::microseconds 	_max_irreversible_transaction_age_us;
		bool					_send_propose_enabled = false;
		string 				_peer_chain_address;
		string 				_peer_chain_account;
		string 				_peer_chain_constract;
		string 				_my_chain_constract;


		optional<boost::signals2::scoped_connection> accepted_transaction_connection;
		optional<boost::signals2::scoped_connection> sync_block_transaction_connection;
		optional<boost::signals2::scoped_connection> irreversible_block_connection;

		void accepted_transaction(const transaction_metadata_ptr& trx) {
			auto& chain = chain_plug->chain();
			auto& db = chain.db();

			auto block_num = chain.pending_block_state()->block_num;
			auto now = fc::time_point::now();
			auto block_age = (chain.pending_block_time() > now) ? fc::microseconds(0) : (now - chain.pending_block_time());

			if (!_send_propose_enabled) {
				return;
			} else if ( _max_irreversible_transaction_age_us.count() >= 0 && block_age >= _max_irreversible_transaction_age_us ) {
				return;
			}

			for (const auto action : trx->trx.actions) {
				if (action.account == name(_my_chain_constract)
					&& action.name == N(transfer)) {
					const auto* existed = db.find<transaction_reversible_object, by_trx_id>(trx->id);
					if (existed != nullptr) {
						return;
					}

					const auto transaction_reversible = db.create<transaction_reversible_object>([&](auto& tso) {
						tso.block_num = block_num;
						tso.trx_id = trx->id;
						tso.data = action.data;
					});
					break;

				} else if (action.account == name(_my_chain_constract)
						   && action.name == N(msigtrans)) {

					auto data = fc::raw::unpack<cactus_msigtrans>(action.data);

					const auto* existed = db.find<transaction_executed_object, by_trx_id>(data.trx_id);

					if(existed != nullptr) {
						db.modify(existed[0] ,[&] (auto& tso) {
							++tso.count;
							tso.timestamp = chain.pending_block_time();
						});
					}else {
						const auto transaction_executed = db.create<transaction_executed_object>([&](auto &tso) {
							tso.trx_id = data.trx_id;
							tso.data = action.data;
							tso.count = 1;
							tso.timestamp = chain.pending_block_time();
							wlog("捕获一条转账 ${block_num}",("block_num",block_num));
						});
					}
					break;

				}
			}
		}

		void apply_irreversible_transaction(const block_state_ptr& irb) {
			auto& chain = chain_plug->chain();
			auto& db = chain.db();

			const auto& trmi = db.get_index<transaction_reversible_multi_index, by_block_num>();
			auto itr = trmi.begin();
			while( itr != trmi.end()) {
				if (itr->block_num <= irb->block_num) {
					auto data = fc::raw::unpack<cactus_transfer>(itr->data);
					// need to validate ?

					// send propose or approve
					string datastr = DATA_FORMAT(_peer_chain_account, string(itr->trx_id), string(data.from), string(data.to), data.quantity.to_string());
					vector<string> permissions = {_peer_chain_account};
					try {
						app().find_plugin<client_plugin>()->get_client_apis().push_action(_peer_chain_address, _peer_chain_constract,
																						  "msigtrans", datastr, permissions);
					} catch (...) {
						wlog("send transfer transaction failed");
					}

					// remove or move to other table ?
					db.remove(*itr);
				}
				++ itr;
			}

			const auto &temi = db.get_index<transaction_executed_multi_index, by_timestamp>();
			auto irreversible_block_num = irb;

			auto titr = temi.begin();
			while (titr != temi.end()) {
				if (titr->timestamp <= irb->header.timestamp.to_time_point() && titr->count < 2) {
					if(irb->header.timestamp.to_time_point().sec_since_epoch() - titr->timestamp.sec_since_epoch() > 60) {
						auto data = fc::raw::unpack<cactus_msigtrans>(titr->data);
						string datastr = DATA_FORMAT(_peer_chain_account, string(data.trx_id), string(data.to), string(data.from), data.quantity.to_string());
						vector<string> permissions = {_peer_chain_account};
						try {
							app().find_plugin<client_plugin>()->get_client_apis().push_action(_peer_chain_address,
																							  _peer_chain_constract,
																							  "msigtrans", datastr,
																							  permissions);
						} catch (...) {
							wlog("send re-transfer transaction failed");
						}

						db.remove(*titr);
					}
				}
				++titr;
			}
		}
	};

	sync_plugin::sync_plugin()
			:my(std::make_shared<sync_plugin_impl>()) {
	}

	sync_plugin::~sync_plugin() {
	}

	void sync_plugin::set_program_options(options_description& cli, options_description& cfg) {
		cfg.add_options()
				("max-irreversible-transaction-age", bpo::value<int32_t>()->default_value( 600 ), "Max irreversible age of transaction to deal")
				("enable-send-propose", bpo::bool_switch()->notifier([this](bool e){my->_send_propose_enabled = e;}), "Enable push propose.")
				("peer-chain-address", bpo::value<string>()->default_value("http://127.0.0.1:8899/"), "In MainChain it is SideChain address, otherwise it's MainChain address")
				("peer-chain-account", bpo::value<string>()->default_value("cactus"), "In MainChain it is your SideChain's account, otherwise it's your MainChain's account")
				("peer-chain-constract", bpo::value<string>()->default_value("cactus"), "In MainChain it is SideChain's cactus constract, otherwise it's MainChain's cactus constract")
				("my-chain-constract", bpo::value<string>()->default_value("cactus"), "this chain's cactus contract")
				;
	}

	void sync_plugin::plugin_initialize(const variables_map& options) {
		try {
			my->_max_irreversible_transaction_age_us = fc::seconds(options.at("max-irreversible-transaction-age").as<int32_t>());
			my->_peer_chain_address = options.at("peer-chain-address").as<string>();
			my->_peer_chain_account = options.at("peer-chain-account").as<string>();
			my->_peer_chain_constract = options.at("peer-chain-constract").as<string>();
			my->_my_chain_constract = options.at("my-chain-constract").as<string>();

			my->chain_plug = app().find_plugin<chain_plugin>();
			auto& chain = my->chain_plug->chain();

			chain.db().add_index<transaction_reversible_multi_index>();
			chain.db().add_index<transaction_executed_multi_index>();

			my->sync_block_transaction_connection.emplace(chain.sync_block_transaction.connect( [&](const transaction_metadata_ptr& trx) {
				my->accepted_transaction(trx);
			}));
			my->irreversible_block_connection.emplace(chain.irreversible_block.connect( [&](const block_state_ptr& irb) {
				my->apply_irreversible_transaction(irb);
			}));

		} FC_LOG_AND_RETHROW()
	}

	void sync_plugin::plugin_startup() {
	}

	void sync_plugin::plugin_shutdown() {
		my->accepted_transaction_connection.reset();
		my->sync_block_transaction_connection.reset();
		my->irreversible_block_connection.reset();
	}


}