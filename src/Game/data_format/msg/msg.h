#pragma once
#include <QString>
#include <map>
#include <vector>
#include <cstdint>


namespace DataFormat::Msg {

struct Message {
  uint32_t id;
  QString context;
  QString text;
};

struct Messages {
	using ItemsArray = std::vector<Message>;
	using Items = std::map<uint32_t, Message>;

	Items items;
};

[[nodiscard]] Messages::ItemsArray to_array(Messages::Items &items);

} // namespace DataFormat::Msg
