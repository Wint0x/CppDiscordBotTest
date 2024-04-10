#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <unordered_map>
#include <sys/stat.h>
#include <time.h>
#include <vector>
#include <string_view>

#include <dpp/dpp.h>

using channel_info = std::unordered_map<std::string, std::string>;

inline void add_to_map(channel_info& details, std::pair<std::string, std::string>&& pair)
{
	auto [key, value] = std::move(pair);

	details[std::move(key)] = std::move(value);
}

inline std::string bool_to_s(const bool v) noexcept
{
	return (v ? "true" : "false");
}

inline std::string convert_unix_timestamp_to_human_readable_date(double timestamp)
{
	time_t timestamp_t = timestamp; 
	auto date = ctime(&timestamp_t);
	return std::string(date);
}

int main(void)
{
	// Begin read token
	struct stat statbuf{};
	if (stat("token.txt", &statbuf) == -1)
	{
		std::cerr << "Token file not found (token.txt)\n";
		return EXIT_FAILURE;
	}

	std::ifstream r_token("token.txt");

	if (!r_token.good())
	{
		std::cerr << "Error reading token file (ifstream)\n";
		return EXIT_FAILURE;
	}

	std::string token_str;
	std::getline(r_token, token_str);

	r_token.close();
	// End of read token

	/* Bot client */
	dpp::cluster bot(token_str, dpp::i_unverified_default_intents );
	
	bot.on_log(dpp::utility::cout_logger());

	// * * * All Bot's Slash Commands Here * * * 
	bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) 
	{
		if (event.command.get_command_name() == "ping")
		{
			event.reply("Pong!");
		}

		if (event.command.get_command_name() == "members")
		{
			std::stringstream memberstream;
			std::vector<std::string> robots;

			std::string emoji{":person_standing:"};

			const auto& this_guild = event.command.get_guild();

			for (const auto& [snowflake_id, guild_member] : this_guild.members)
			{
				if (guild_member.get_user()->is_bot())
				{
					robots.emplace_back(guild_member.get_user()->username);
					continue;
				}

				memberstream << emoji << " - " << "**" << guild_member.get_user()->username << "**" << '\n';
			}

			emoji = ":robot:";
			for (const auto& bot : robots)
			{
				memberstream << emoji << " - " << "**" << bot << "**" << '\n';
			}

			robots.clear();
			event.reply(memberstream.str());
		}

		if (event.command.get_command_name() == "channel_info")
		{
			// Get info
			const auto& c = event.command.get_channel();
			channel_info details;

			add_to_map(details, std::make_pair<std::string, std::string>("mention", c.get_mention()));
			add_to_map(details, std::make_pair<std::string, std::string>("icon", c.get_icon_url()));
			add_to_map(details, std::make_pair<std::string, std::string>("url", c.get_url()));
			add_to_map(details, std::make_pair<std::string, std::string>("NSFW", bool_to_s(c.is_nsfw())));
			add_to_map(details, std::make_pair<std::string, std::string>("text channel", bool_to_s(c.is_text_channel())));
			add_to_map(details, std::make_pair<std::string, std::string>("forum", bool_to_s(c.is_forum())));
			add_to_map(details, std::make_pair<std::string, std::string>("video auto", bool_to_s(c.is_video_auto())));
			add_to_map(details, std::make_pair<std::string, std::string>("creation time", std::to_string(c.get_creation_time())));

			add_to_map(details, std::make_pair<std::string, std::string>("name", std::string(c.name)));
			add_to_map(details, std::make_pair<std::string, std::string>("topic", std::string(c.topic)));
			add_to_map(details, std::make_pair<std::string, std::string>("rtc region", std::string(c.rtc_region)));
			add_to_map(details, std::make_pair<std::string, std::string>("user limit", std::to_string(c.user_limit)));

			add_to_map(details, std::make_pair<std::string, std::string>("ID", c.id.str()));
			add_to_map(details, std::make_pair<std::string, std::string>("owner id", c.owner_id.str()));
			add_to_map(details, std::make_pair<std::string, std::string>("guild id", c.guild_id.str()));

			std::stringstream infostream;
			std::string title = "**" + details["name"] + " Channel Info**"; 

			infostream << "**Url**: " << details["url"] << '\n'\
				<< "**Topic**: " << ((details["topic"].empty() ? "None" : details["topic"])) << '\n'\
				<< "**Channel ID**: " << details["ID"] << '\n'\
				<< "**Guild ID**: " << details["guild id"] << '\n'\
				<< "**Owner ID**: " << details["owner id"] << '\n'\
				<< "**User Limit**: " << details["user limit"] << '\n'\
				<< "**RTC Region**: " << ((details["rtc region"].empty() ? "None" : details["rtc region"])) << '\n'\
				<< "**Text Channel?**: " << details["text channel"] << '\n'\
				<< "**Forum Channel?**: " << details["forum"] << '\n'\
				<< "**NSFW Channel?**: " << details["NSFW"] << '\n'\
				<< "**Creation Time**: " << convert_unix_timestamp_to_human_readable_date(c.get_creation_time()) << '\n';

			const auto& guild_icon_url = event.command.get_guild().get_icon_url();
			dpp::embed embed;

			embed.set_title(title)
				.set_description(infostream.str())
				.set_thumbnail(guild_icon_url)
				.set_color(0x4CC417);

			event.reply(embed);
		}

		if (event.command.get_command_name() == "purge")
		{
			int64_t amount = std::get<int64_t>(event.get_parameter("amount"));

			bot.messages_get(event.command.channel_id,0,0,0,amount + 1, [&bot, event](const dpp::confirmation_callback_t& callback)
			{
				if (callback.is_error())
				{	
					event.reply(callback.get_error().message);
					return;
				}
				
				auto messages = callback.get<dpp::message_map>();
				std::vector<dpp::snowflake> message_ids;
				for (const auto& pair : messages)
					message_ids.emplace_back(pair.first);

				if (!message_ids.empty())
				{	
					bot.message_delete_bulk(message_ids, event.command.channel_id, [event](const dpp::confirmation_callback_t& callback)
					{
						if (callback.is_error())
						{
							event.reply(callback.get_error().message);
							return;
						}

						event.reply(":green_square: **Done**");
					});
				}

				else
					event.reply("Nothing to delete...");
			});
		}

		if (event.command.get_command_name() == "channel_name")
		{
			const std::string new_channel_name = std::get<std::string>(event.get_parameter("name"));
			const std::string old_channel_name = event.command.get_channel().name;

			bot.channel_get(event.command.channel_id, [&bot, &old_channel_name, &new_channel_name, event](const dpp::confirmation_callback_t& callback)
			{
				if (callback.is_error())
				{
					event.reply(callback.get_error().message);
					return;
				}

				auto channel_to_edit = callback.get<dpp::channel>();
				channel_to_edit.set_name(new_channel_name);
				bot.channel_edit(channel_to_edit);

				event.reply(":green_square: **Changed channel name \"" + old_channel_name + "\" to \"" + new_channel_name + "\"**");
			});
		}

	});

	// Make bot send a message when reacting to one of its messages
	bot.on_message_reaction_add([&bot](const dpp::message_reaction_add_t& event)
	{
		auto& reaction_author = event.reacting_user;
		const auto& message_author_id = event.message_author_id;
		
		if ((!reaction_author.is_bot()) && message_author_id == bot.me.id)
		{
			std::string reaction_author_user_name = reaction_author.format_username();
			dpp::message response(event.channel_id, std::string("Thank you for reacting, ").append(reaction_author_user_name));

			bot.message_create(response);
		}
	});

	bot.on_message_create([&bot](const dpp::message_create_t& message_event)
	{
		const auto message = dpp::lowercase(message_event.msg.content);
		const dpp::snowflake channel_id = message_event.msg.channel_id;

		if (!message_event.msg.author.is_bot())
		{
			// Just for testing...
			if (message.starts_with("?find_user "))
			{
				std::string user_id = message.substr(message.find_first_of(' ') + 1, message.length());
				dpp::snowflake get_user_by_id;

				try
				{
					get_user_by_id = std::stoul(user_id);
					
					bot.user_get(get_user_by_id, [&message_event](const dpp::confirmation_callback_t& callback)
						{
							if (callback.is_error())
								message_event.reply(callback.get_error().message);

							else
							{
								dpp::user_identified user_found = callback.get<dpp::user_identified>();
								message_event.reply("Found user: " + user_found.get_mention());
							}
					});

				}
				catch (...)
				{
					message_event.reply("Something went wrong, probably an invalid user_id!");
				}
			}
		}
	});

	bot.on_ready([&bot](const dpp::ready_t& event)
	{
		if (dpp::run_once<struct register_bot_commands>())
		{
			/* */
			bot.global_command_create(dpp::slashcommand("ping", "Ping Pong!", bot.me.id));
			bot.global_command_create(dpp::slashcommand("channel_info", "Gather channel info!", bot.me.id));
			bot.global_command_create(dpp::slashcommand("members", "Shows the list of server members!", bot.me.id));

			/* */
			constexpr int min_amount = 1;
			constexpr int max_amount = 200;
			dpp::slashcommand purge("purge", "Delete a specified amount of messages in current channel", bot.me.id);
			purge.add_option(
				dpp::command_option(dpp::co_integer, "amount", "The desired amount (max 200)", true)
				.set_min_value(min_amount)
				.set_max_value(max_amount)
				);

			bot.global_command_create(purge);

			/* Not Defined */
			dpp::slashcommand edit_channel_name("channel_name", "Change the name of the current channel.", bot.me.id);
			edit_channel_name.add_option(
				dpp::command_option(dpp::co_string, "name", "The new channel's name!", true)
				);
			edit_channel_name.set_default_permissions(dpp::permissions::p_manage_channels);

			bot.global_command_create(edit_channel_name);
		}

		dpp::presence presence(dpp::presence_status::ps_online, dpp::activity_type::at_competing, "Non-stop C++ coding!!!");
		bot.set_presence(presence);
	});

	bot.start(dpp::st_wait);

	return EXIT_SUCCESS;
}