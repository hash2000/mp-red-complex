#pragma once

enum class ProcedureId {
  Timed = 0x01,
  Conditional = 0x02,
  Imported = 0x04,
  Exported = 0x08,
  Critical = 0x10,
  Pure = 0x20,
  Inline = 0x40,
};
