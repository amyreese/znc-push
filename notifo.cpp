/**
 * ZNC Notifo Module
 *
 * Allows the user to enter a Notifo user and API token, and sends
 * channel highlights and personal messages to Notifo.
 *
 * Copyright (c) 2011 John Reese
 * Licensed under the MIT license
 */

#define REQUIRESSL

#include "znc.h"
#include "Chan.h"
#include "User.h"
#include "Modules.h"
#include "time.h"

#if (!defined(VERSION_MAJOR) || !defined(VERSION_MINOR) || (VERSION_MAJOR == 0 && VERSION_MINOR < 72))
#error This module needs ZNC 0.072 or newer.
#endif

// Handle versions of ZNC older than 0.090 by disabling the away_only condition
#if VERSION_MAJOR == 0 && VERSION_MINOR >= 90
#define NOTIFO_AWAY
#endif

// Debug output
#define NOTIFO_DEBUG 0

#if NOTIFO_DEBUG
#define PutDebug(s) PutModule(s)
#else
#define PutDebug(s) //s
#endif

class CNotifoMod : public CModule
{
	protected:

		// Application name
		CString app;

		// Too lazy to add CString("\r\n\") everywhere
		CString crlf;

		// BASIC auth string, needs to be encoded each time username/secret is changed
		CString notifo_auth;

		// Host and URL to send messages to
		CString notifo_host;
		CString notifo_url;

		// User agent to use
		CString user_agent;

		// Time last notification was sent for a given context
		map <CString, unsigned int> last_notification_time;

		// Time of last message by user to a given context
		map <CString, unsigned int> last_reply_time;

		// Time of last activity by user for a given context
		map <CString, unsigned int> last_active_time;

		// Time of last activity by user in any context
		unsigned int idle_time;

		// User object
		CUser *user;

		// Configuration options
		MCString options;
		MCString defaults;

	public:

		MODCONSTRUCTOR(CNotifoMod) {
			app = "ZNC";
			crlf = "\r\n";

			idle_time = time(NULL);
			notifo_auth = "";
			notifo_host = "api.notifo.com";
			notifo_url = "/v1/send_notification";
			user_agent = "ZNC To Notifo";

			// Current user
			user = GetUser();

			// Notifo user account and secret
			defaults["username"] = "";
			defaults["secret"] = "";

			// Condition strings
			defaults["channel_conditions"] = "all";
			defaults["query_conditions"] = "all";

			// Notification conditions
#ifdef NOTIFO_AWAY
			defaults["away_only"] = "no";
#endif
			defaults["client_count_less_than"] = "0";
			defaults["highlight"] = "";
			defaults["idle"] = "0";
			defaults["last_active"] = "180";
			defaults["last_notification"] = "300";
			defaults["nick_blacklist"] = "";
			defaults["replied"] = "yes";

			// Notification settings
			defaults["message_length"] = "100";
			defaults["message_uri"] = "";
		}
		virtual ~CNotifoMod() {}

	protected:

		/**
		 * Shorthand for encoding a string for a URL.
		 *
		 * @param str String to be encoded
		 * @return Encoded string
		 */
		CString urlencode(const CString& str)
		{
			return str.Escape_n(CString::EASCII, CString::EURL);
		}

		/**
		 * Re-encode the authentication credentials.
		 */
		void authencode()
		{
			// BASIC auth, base64-encoded username:password
			CString auth = options["username"] + CString(":") + options["secret"];
			notifo_auth = auth.Base64Encode_n();
		}

		/**
		 * Performs string expansion on a set of keywords.
		 * Given an initial string and a dictionary of string replacments,
		 * iterate over the dictionary, expanding keywords one-by-one.
		 *
		 * @param content String contents
		 * @param replace Dictionary of string replacements
		 * @return Result of string replacements
		 */
		CString expand(const CString& content, MCString& replace)
		{
			CString result = content.c_str();

			for(MCString::iterator i = replace.begin(); i != replace.end(); i++)
			{
				result.Replace(i->first, i->second);
			}

			return result;
		}

		/**
		 * Send a message to the currently-configured Notifo account.
		 * Requires (and assumes) that the user has already configured their
		 * username and API secret using the 'set' command.
		 *
		 * @param message Message to be sent to the user
		 * @param title Message title to use
		 * @param context Channel or nick context
		 */
		void send_message(const CString& message, const CString& title="New Message", const CString& context="*notifo", const CNick& nick=CString("*notifo"))
		{
			// Set the last notification time
			last_notification_time[context] = time(NULL);

			// Shorten message if needed
			unsigned int message_length = options["message_length"].ToUInt();
			CString short_message = message;
			if (message_length > 0)
			{
				short_message = message.Ellipsize(message_length);
			}

			// Generate an ISO8601 date string
			time_t rawtime;
			struct tm * timeinfo;
			time(&rawtime);
			timeinfo = localtime(&rawtime);
			char iso8601 [20];
			strftime(iso8601, 20, "%Y-%m-%d %H:%M:%S", timeinfo);

			// URI string replacements
			MCString replace;
			replace["{context}"] = context;
			replace["{nick}"] = nick.GetNick();
			replace["{datetime}"] = CString(iso8601);
			replace["{unixtime}"] = CString(time(NULL));
			CString uri = expand(options["message_uri"], replace);

			// POST body parameters for the request
			CString post = "to=" + urlencode(options["username"]);
			post += "&msg=" + urlencode(short_message);
			post += "&label=" + urlencode(app);
			post += "&title=" + urlencode(title);
			post += "&uri=" + urlencode(uri);

			// Request headers and POST body
			CString request = "POST " + notifo_url + " HTTP/1.1" + crlf;
			request += "Host: " + notifo_host + crlf;
			request += "Content-Type: application/x-www-form-urlencoded" + crlf;
			request += "Content-Length: " + CString(post.length()) + crlf;
			request += "User-Agent: " + user_agent + crlf;
			request += "Authorization: Basic " + notifo_auth + crlf;
			request += crlf;
			request += post + crlf;

			// Create the socket connection, write to it, and add it to the queue
			CSocket *sock = new CSocket(this);
			sock->Connect(notifo_host, 443, true);
			sock->Write(request);
			sock->Close(Csock::CLT_AFTERWRITE);
			AddSocket(sock);
		}

		/**
		 * Evaluate a boolean expression using condition values.
		 * All tokens must be separated by spaces, using "and" and "or" for
		 * boolean operators, "(" and ")" to enclose sub-expressions, and
		 * condition option names to evaluate each condition.
		 *
		 * @param expression Boolean expression string
		 * @param context Notification context
		 * @param nick Sender nick
		 * @param message Message contents
		 * @return Result of boolean evaluation
		 */
		bool eval(const CString& expression, const CString& context=CString(""), const CNick& nick=CNick(""), const CString& message=" ")
		{
			CString padded = expression.Replace_n("(", " ( ");
			padded.Replace(")", " ) ");

			VCString tokens;
			padded.Split(" ", tokens, false);

			PutDebug("Evaluating message: <" + nick.GetNick() + "> " + message);
			bool result = eval_tokens(tokens.begin(), tokens.end(), context, nick, message);

			return result;
		}

#define expr(x, y) else if (token == x) { \
	bool result = y; \
	dbg += CString(x) + "/" + CString(result ? "true" : "false") + " "; \
	value = oper ? value && result : value || result; \
}

		/**
		 * Evaluate a tokenized boolean expression, or sub-expression.
		 *
		 * @param pos Token vector iterator current position
		 * @param end Token vector iterator end position
		 * @param context Notification context
		 * @param nick Sender nick
		 * @param message Message contents
		 * @return Result of boolean expression
		 */
		bool eval_tokens(VCString::iterator pos, VCString::iterator end, const CString& context, const CNick& nick, const CString& message)
		{
			bool oper = true;
			bool value = true;

			CString dbg = "";

			for(; pos != end; pos++)
			{
				CString token = pos->AsLower();

				if (token == "(")
				{
					// recursively evaluate sub-expressions
					bool inner = eval_tokens(++pos, end, context, nick, message);
					dbg += "( inner/" + CString(inner ? "true" : "false") + " ) ";
					value = oper ? value && inner : value || inner;

					// search ahead to the matching parenthesis token
					unsigned int parens = 1;
					while(pos != end)
					{
						if (*pos == "(")
						{
							parens++;
						}
						else if (*pos == ")")
						{
							parens--;
						}

						if (parens == 0)
						{
							break;
						}

						pos++;
					}
				}
				else if (token == ")")
				{
					pos++;
					PutDebug(dbg);
					return value;
				}
				else if (token == "and")
				{
					dbg += "and ";
					oper = true;
				}
				else if (token == "or")
				{
					dbg += "or ";
					oper = false;
				}

				expr("true", true)
				expr("false", false)
				expr("away_only", away_only())
				expr("client_count_less_than", client_count_less_than())
				expr("highlight", highlight(message))
				expr("idle", idle())
				expr("last_active", last_active(context))
				expr("last_notification", last_notification(context))
				expr("nick_blacklist", nick_blacklist(nick))
				expr("replied", replied(context))

				else
				{
					PutModule("Error: Unexpected token \"" + token + "\"");
				}
			}

			PutDebug(dbg);
			return value;
		}

#undef expr

	protected:

		/**
		 * Check if the away status condition is met.
		 *
		 * @return True if away_only is not "yes" or away status is set
		 */
		bool away_only()
		{
#ifdef NOTIFO_AWAY
			CString value = options["away_only"].AsLower();
			return value != "yes" || user->IsIRCAway();
#else
			return true
#endif
		}

		/**
		 * Check how many clients are connected to ZNC.
		 *
		 * @return Number of connected clients
		 */
		unsigned int client_count()
		{
			return user->GetClients().size();
		}

		/**
		 * Check if the client_count condition is met.
		 *
		 * @return True if client_count is less than client_count_less_than or if client_count_less_than is zero
		 */
		bool client_count_less_than()
		{
			unsigned int value = options["client_count_less_than"].ToUInt();
			return value == 0 || client_count() < value;
		}

		/**
		 * Determine if the given message matches any highlight rules.
		 *
		 * @param message Message contents
		 * @return True if message matches a highlight
		 */
		bool highlight(const CString& message)
		{
			CString msg = " " + message.AsLower() + " ";

			VCString values;
			options["highlight"].Split(" ", values, false);

			for (VCString::iterator i = values.begin(); i != values.end(); i++)
			{
				CString value = i->AsLower();
				char prefix = value[0];
				bool notify = true;

				if (prefix == '-')
				{
					notify = false;
					value.LeftChomp(1);
				}
				else if (prefix == '_')
				{
					value = " " + value.LeftChomp_n(1) + " ";
				}

				value = "*" + value + "*";

				if (msg.WildCmp(value))
				{
					return notify;
				}
			}

			CNick nick = user->GetIRCNick();

			if (message.find(nick.GetNick()) != string::npos)
			{
				return true;
			}

			return false;
		}

		/**
		 * Check if the idle condition is met.
		 *
		 * @return True if idle is zero or elapsed time is greater than idle
		 */
		bool idle()
		{
			unsigned int value = options["idle"].ToUInt();
			unsigned int now = time(NULL);
			return value == 0
				|| idle_time + value < now;
		}

		/**
		 * Check if the last_active condition is met.
		 *
		 * @param context Channel or nick context
		 * @return True if last_active is zero or elapsed time is greater than last_active
		 */
		bool last_active(const CString& context)
		{
			unsigned int value = options["last_active"].ToUInt();
			unsigned int now = time(NULL);
			return value == 0
				|| last_active_time.count(context) < 1
				|| last_active_time[context] + value < now;
		}

		/**
		 * Check if the last_notification condition is met.
		 *
		 * @param context Channel or nick context
		 * @return True if last_notification is zero or elapsed time is greater than last_nofication
		 */
		bool last_notification(const CString& context)
		{
			unsigned int value = options["last_notification"].ToUInt();
			unsigned int now = time(NULL);
			return value == 0
				|| last_notification_time.count(context) < 1
				|| last_notification_time[context] + value < now;
		}

		/**
		 * Check if the nick_blacklist condition is met.
		 *
		 * @param nick Nick that sent the message
		 * @return True if nick is not in the blacklist
		 */
		bool nick_blacklist(const CNick& nick)
		{
			VCString blacklist;
			options["nick_blacklist"].Split(" ", blacklist, false);

			CString name = nick.GetNick().AsLower();

			for (VCString::iterator i = blacklist.begin(); i != blacklist.end(); i++)
			{
				if (name.WildCmp(i->AsLower()))
				{
					return false;
				}
			}

			return true;
		}

		/**
		 * Check if the replied condition is met.
		 *
		 * @param context Channel or nick context
		 * @return True if last_reply_time > last_notification_time or if replied is not "yes"
		 */
		bool replied(const CString& context)
		{
			CString value = options["replied"].AsLower();
			return value != "yes"
				|| last_notification_time[context] == 0
				|| last_notification_time[context] < last_reply_time[context];
		}

		/**
		 * Determine when to notify the user of a channel message.
		 *
		 * @param nick Nick that sent the message
		 * @param channel Channel the message was sent to
		 * @param message Message contents
		 * @return Notification should be sent
		 */
		bool notify_channel(const CNick& nick, const CChan& channel, const CString& message)
		{
			CString context = channel.GetName();

			CString expression = options["channel_conditions"].AsLower();
			if (expression != "all")
			{
				return eval(expression, context, nick, message);
			}

			return away_only()
				&& client_count_less_than()
				&& highlight(message)
				&& idle()
				&& last_active(context)
				&& last_notification(context)
				&& nick_blacklist(nick)
				&& replied(context)
				&& true;
		}

		/**
		 * Determine when to notify the user of a private message.
		 *
		 * @param nick Nick that sent the message
		 * @return Notification should be sent
		 */
		bool notify_pm(const CNick& nick, const CString& message)
		{
			CString context = nick.GetNick();

			CString expression = options["query_conditions"].AsLower();
			if (expression != "all")
			{
				return eval(expression, context, nick, message);
			}

			return away_only()
				&& client_count_less_than()
				&& idle()
				&& last_active(context)
				&& last_notification(context)
				&& nick_blacklist(nick)
				&& replied(context)
				&& true;
		}

	protected:

		/**
		 * Handle the plugin being loaded.  Retrieve plugin config values.
		 *
		 * @param args Plugin arguments
		 * @param message Message to show the user after loading
		 */
		bool OnLoad(const CString& args, CString& message)
		{
			for (MCString::iterator i = defaults.begin(); i != defaults.end(); i++)
			{
				CString value = GetNV(i->first);
				if (value != "")
				{
					options[i->first] = value;
				}
				else
				{
					options[i->first] = defaults[i->first];
				}
			}

			authencode();

			return true;
		}

		/**
		 * Handle channel messages.
		 *
		 * @param nick Nick that sent the message
		 * @param channel Channel the message was sent to
		 * @param message Message contents
		 */
		EModRet OnChanMsg(CNick& nick, CChan& channel, CString& message)
		{
			if (notify_channel(nick, channel, message))
			{
				CString title = "Highlight";
				CString msg = channel.GetName();
				msg += ": <" + nick.GetNick();
				msg += "> " + message;

				send_message(msg, title, channel.GetName());
			}

			return CONTINUE;
		}

		/**
		 * Handle channel actions.
		 *
		 * @param nick Nick that sent the action
		 * @param channel Channel the message was sent to
		 * @param message Message contents
		 */
		EModRet OnChanAction(CNick& nick, CChan& channel, CString& message)
		{
			if (notify_channel(nick, channel, message))
			{
				CString title = "Highlight";
				CString msg = channel.GetName();
				msg += ": " + nick.GetNick();
				msg += " " + message;

				send_message(msg, title, channel.GetName());
			}

			return CONTINUE;
		}

		/**
		 * Handle a private message.
		 *
		 * @param nick Nick that sent the message
		 * @param message Message contents
		 */
		EModRet OnPrivMsg(CNick& nick, CString& message)
		{
			if (notify_pm(nick, message))
			{
				CString title = "Private Message";
				CString msg = "From " + nick.GetNick();
				msg += ": " + message;

				send_message(msg, title, nick.GetNick());
			}

			return CONTINUE;
		}

		/**
		 * Handle a private action.
		 *
		 * @param nick Nick that sent the action
		 * @param message Message contents
		 */
		EModRet OnPrivAction(CNick& nick, CString& message)
		{
			if (notify_pm(nick, message))
			{
				CString title = "Private Message";
				CString msg = "* " + nick.GetNick();
				msg += " " + message;

				send_message(msg, title, nick.GetNick());
			}

			return CONTINUE;
		}

		/**
		 * Handle a message sent by the user.
		 *
		 * @param target Target channel or nick
		 * @param message Message contents
		 */
		EModRet OnUserMsg(CString& target, CString& message)
		{
			last_reply_time[target] = last_active_time[target] = idle_time = time(NULL);
			return CONTINUE;
		}

		/**
		 * Handle an action sent by the user.
		 *
		 * @param target Target channel or nick
		 * @param message Message contents
		 */
		EModRet OnUserAction(CString& target, CString& message)
		{
			last_reply_time[target] = last_active_time[target] = idle_time = time(NULL);
			return CONTINUE;
		}

		/**
		 * Handle the user joining a channel.
		 *
		 * @param channel Channel name
		 * @param key Channel key
		 */
		EModRet OnUserJoin(CString& channel, CString& key)
		{
			idle_time = time(NULL);
			return CONTINUE;
		}

		/**
		 * Handle the user parting a channel.
		 *
		 * @param channel Channel name
		 * @param message Part message
		 */
		EModRet OnUserPart(CString& channel, CString& message)
		{
			idle_time = time(NULL);
			return CONTINUE;
		}

		/**
		 * Handle the user setting the channel topic.
		 *
		 * @param channel Channel name
		 * @param topic Topic message
		 */
		EModRet OnUserTopic(CString& channel, CString& topic)
		{
			idle_time = time(NULL);
			return CONTINUE;
		}

		/**
		 * Handle the user requesting the channel topic.
		 *
		 * @param channel Channel name
		 */
		EModRet OnUserTopicRequest(CString& channel)
		{
			idle_time = time(NULL);
			return CONTINUE;
		}

		/**
		 * Handle direct commands to the *notifo virtual user.
		 *
		 * @param command Command string
		 */
		void OnModCommand(const CString& command)
		{
			VCString tokens;
			int token_count = command.Split(" ", tokens, false);

			if (token_count < 1)
			{
				return;
			}

			CString action = tokens[0].AsLower();

			// SET command
			if (action == "set")
			{
				if (token_count < 3)
				{
					PutModule("Usage: set <option> <value>");
					return;
				}

				CString option = tokens[1].AsLower();
				CString value = command.Token(2, true, " ");
				MCString::iterator pos = options.find(option);

				if (pos == options.end())
				{
					PutModule("Error: invalid option name");
				}
				else
				{
					if (option == "channel_conditions" || option == "query_conditions")
					{
						if (value != "all")
						{
							eval(value);
						}
					}

					options[option] = value;
					options[option].Trim();
					SetNV(option, options[option]);

					authencode();
				}
			}
			// APPEND command
			else if (action == "append")
			{
				if (token_count < 3)
				{
					PutModule("Usage: append <option> <value>");
					return;
				}

				CString option = tokens[1].AsLower();
				CString value = command.Token(2, true, " ");
				MCString::iterator pos = options.find(option);

				if (pos == options.end())
				{
					PutModule("Error: invalid option name");
				}
				else
				{
					options[option] += " " + value;
					options[option].Trim();
					SetNV(option, options[option]);

					authencode();
				}
			}
			// PREPEND command
			else if (action == "prepend")
			{
				if (token_count < 3)
				{
					PutModule("Usage: prepend <option> <value>");
					return;
				}

				CString option = tokens[1].AsLower();
				CString value = command.Token(2, true, " ");
				MCString::iterator pos = options.find(option);

				if (pos == options.end())
				{
					PutModule("Error: invalid option name");
				}
				else
				{
					options[option] = value + " " + options[option];
					options[option].Trim();
					SetNV(option, options[option]);

					authencode();
				}
			}
			// UNSET command
			else if (action == "unset")
			{
				if (token_count != 2)
				{
					PutModule("Usage: unset <option>");
					return;
				}

				CString option = tokens[1].AsLower();
				MCString::iterator pos = options.find(option);

				if (pos == options.end())
				{
					PutModule("Error: invalid option name");
				}
				else
				{
					options[option] = defaults[option];
					DelNV(option);

					authencode();
				}
			}
			// GET command
			else if (action == "get")
			{
				if (token_count > 2)
				{
					PutModule("Usage: get [<option>]");
					return;
				}

				if (token_count < 2)
				{
					CTable table;

					table.AddColumn("Option");
					table.AddColumn("Value");

					for (MCString::iterator i = options.begin(); i != options.end(); i++)
					{
						table.AddRow();
						table.SetCell("Option", i->first);
						table.SetCell("Value", i->second);
					}

					PutModule(table);
					return;
				}

				CString option = tokens[1].AsLower();
				MCString::iterator pos = options.find(option);

				if (pos == options.end())
				{
					PutModule("Error: invalid option name");
				}
				else
				{
					PutModule(option + CString(": \"") + options[option] + CString("\""));
				}
			}
			// STATUS command
			else if (action == "status")
			{
				CTable table;

				table.AddColumn("Condition");
				table.AddColumn("Status");

#ifdef NOTIFO_AWAY
				table.AddRow();
				table.SetCell("Condition", "away");
				table.SetCell("Status", user->IsIRCAway() ? "yes" : "no");
#endif

				table.AddRow();
				table.SetCell("Condition", "client_count");
				table.SetCell("Status", CString(client_count()));

				unsigned int now = time(NULL);
				unsigned int ago = now - idle_time;

				table.AddRow();
				table.SetCell("Condition", "idle");
				table.SetCell("Status", CString(ago) + " seconds");

				if (token_count > 1)
				{
					CString context = tokens[1];

					table.AddRow();
					table.SetCell("Condition", "last_active");

					if (last_active_time.count(context) < 1)
					{
						table.SetCell("Status", "n/a");
					}
					else
					{
						ago = now - last_active_time[context];
						table.SetCell("Status", CString(ago) + " seconds");
					}

					table.AddRow();
					table.SetCell("Condition", "last_notification");

					if (last_notification_time.count(context) < 1)
					{
						table.SetCell("Status", "n/a");
					}
					else
					{
						ago = now - last_notification_time[context];
						table.SetCell("Status", CString(ago) + " seconds");
					}

					table.AddRow();
					table.SetCell("Condition", "replied");
					table.SetCell("Status", replied(context) ? "yes" : "no");
				}

				PutModule(table);
			}
			// SEND command
			else if (action == "send")
			{
				CString message = command.Token(1, true, " ", true);
				send_message(message);
			}
			// HELP command
			else if (action == "help")
			{
				PutModule("View the detailed documentation at https://github.com/jreese/znc-notifo/blob/master/README.md");
			}
			// EVAL command
			else if (action == "eval")
			{
				CString value = command.Token(1, true, " ");
				PutModule(eval(value) ? "true" : "false");
			}
			else
			{
				PutModule("Error: invalid command, try `help`");
			}
		}
};

MODULEDEFS(CNotifoMod, "Send highlights and personal messages to a Notifo account")
