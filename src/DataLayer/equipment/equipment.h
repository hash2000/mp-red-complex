#pragma once
#include <QString>
#include <list>

// слот, на котором находится экиперовка
enum class EquipmentSlotType {
	Head,
	Body,
	WeaponLeft,
	WeaponRight,
	GlovesLeft,
	GlovesRight,
	Boots,
	RingLeft,
	RingRight,
	Amulet,
	Backpack,
	Bag1,
	Bag2,
};

class EquipmentItem {
public:
	QString id;
	EquipmentSlotType slot;
};

class Equipment {
public:
	QString id;
	std::list<EquipmentItem> items;
};
