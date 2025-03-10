// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/logger.h"
#include "utils/string_array.h"

StringArray::StringArray (const char * const * strs)
{
  int          count = 0;
  const char * s;
  while (strs[count] != nullptr)
    {
      s = strs[count];

      add (juce::CharPointer_UTF8 (s));
      count++;
    }
}

StringArray::StringArray (const QStringList &qlist)
{
  for (const auto &s : qlist)
    {
      add (s.toStdString ());
    }
}
void
StringArray::print (std::string title) const
{
  std::string str = title + ":";
  for (auto &cur_str : arr_)
    {
      str += " " + cur_str.toStdString () + " |";
    }
  str.erase (str.size () - 1);
  z_info ("{}", str);
}

QStringList
StringArray::toQStringList () const
{
  QStringList ret;
  for (const auto &s : arr_)
    {
      ret.append (QString::fromStdString (s.toStdString ()));
    }
  return ret;
}

std::vector<std::string>
StringArray::toStdStringVector () const
{
  std::vector<std::string> ret;
  for (const auto &s : arr_)
    {
      ret.push_back (s.toStdString ());
    }
  return ret;
}
