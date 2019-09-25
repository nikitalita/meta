﻿#include "agent.h"
#include "nodes.h"
namespace behavior
{
	std::unordered_map<std::string, const btree_desc*> btree_desc::tree_cache = {};
	std::string btree_desc::btree_respository = "";
	bool agent::poll()
	{
		if (during_poll)
		{
			return false;
		}
		during_poll = true;
		std::size_t poll_count = 0;
		if (_debug_on)
		{
			_logger->info("poll begins with fronts:");
			for (const auto one_node : _fronts)
			{
				_logger->info("{}", one_node->debug_info());
			}
		}
		while (true)
		{
			if (!_enabled)
			{
				return false;
			}

			bool poll_result = false;
			poll_result |= poll_events();
			poll_result |= poll_fronts();
			if (!reset_flag)
			{
				if (!poll_result)
				{
					break;
				}
			}
			else
			{
				reset_flag = false;
				_fronts.push_back(cur_root_node);
			}
			poll_count += 1;
		}
		during_poll = false;
		if (poll_count)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	bool agent::poll_events()
	{
		if (_events.empty())
		{
			return false;
		}
		for (const auto& one_event : _events)
		{
			for (auto one_node : _fronts)
			{
				if (one_node->handle_event(one_event))
				{
					break;
				}
			}
		}
		_events.clear();
		return true;
		
	}
	bool agent::poll_fronts()
	{
		if (_fronts.empty())
		{
			return false;
		}
		std::swap(pre_fronts, _fronts);
		int ready_count = 0;
		for (const auto& one_node : pre_fronts)
		{
			if (one_node->ready())
			{
				ready_count++;
				poll_node(one_node);
				if (!_enabled)
				{
					break;
					return false;
				}
			}
			else
			{
				_fronts.push_back(one_node);
			}
		}
		pre_fronts.clear();
		return _enabled && ready_count > 0;
	}
	void agent::poll_node(node* cur_node)
	{
		current_poll_node = cur_node;
		if (_debug_on)
		{
			_logger->info("poll node {}", cur_node->debug_info());
		}
		cur_node->visit();
		current_poll_node = nullptr;
	}
	void agent::dispatch_event(const event_type& new_event)
	{
		_events.push_back(new_event);
		poll();
	}
	void agent::notify_stop()
	{
		if (current_poll_node)
		{
			_logger->warn("btree stop at {}", current_poll_node->debug_info());
		}
		else
		{
			_logger->warn("btree stop while current_poll_node empty");
		}
		for (auto one_timer : _timers)
		{
			one_timer.cancel();
		}
		_logger->warn("fronts begin ");
		for (const auto i : pre_fronts)
		{
			_logger->warn(i->debug_info());
			i->interrupt();
		}
		_logger->warn("fronts ends ");
		_fronts.clear();
		_events.clear();
		current_poll_node = nullptr;
		_enabled = false;
	}
	std::optional<bool> agent::agent_action(const std::string& action_name, 
		const meta::serialize::any_vector& action_args)
	{
		return std::nullopt;
	}
	void agent::reset()
	{
		_blackboard.clear();
		
		reset_flag = true;
		notify_stop();
	}
	bool agent::set_debug(bool debug_flag)
	{
		auto pre_flag = _debug_on;
		_debug_on = debug_flag;
		return pre_flag;
	}
	bool agent::load_btree(const std::string& btree_name)
	{
		const btree_desc* cur_btree = btree_desc::load(btree_name);
		if (!cur_btree)
		{
			return false;
		}
		cur_root_node = node_creator::create_node_by_idx(*cur_btree, 0, nullptr);
		if (!cur_root_node)
		{
			return false;
		}
		_fronts.push_back(cur_root_node);
		return true;

	}
	bool agent::enable(bool enable_flag)
	{
		if (enable_flag)
		{
			if (_enabled)
			{
				return true;
			}
			else
			{
				_enabled = true;
				poll();
				return false;
			}
		}
		else
		{
			if (!_enabled)
			{
				return false;
			}
			else
			{
				_enabled = false;
				notify_stop();
				return true;
			}
		}
	}
}