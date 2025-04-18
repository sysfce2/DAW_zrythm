// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __DSP_COLORED_OBJECT_H__
#define __DSP_COLORED_OBJECT_H__

#include "gui/dsp/arranger_object.h"
#include "utils/color.h"

#define DEFINE_COLORED_OBJECT_QML_PROPERTIES(ClassType) \
public: \
  /* ================================================================ */ \
  /* useColor */ \
  /* ================================================================ */ \
  Q_PROPERTY ( \
    bool useColor READ getUseColor WRITE setUseColor NOTIFY useColorChanged) \
  bool getUseColor () const \
  { \
    return use_color_; \
  } \
  void setUseColor (bool use_color) \
  { \
    if (use_color_ == use_color) \
      return; \
    use_color_ = use_color; \
    Q_EMIT useColorChanged (use_color_); \
  } \
  Q_SIGNAL void useColorChanged (bool use_color); \
  /* ================================================================ */ \
  /* color */ \
  /* ================================================================ */ \
  Q_PROPERTY (QColor color READ getColor WRITE setColor NOTIFY colorChanged) \
  QColor getColor () const \
  { \
    return color_.to_qcolor (); \
  } \
  void setColor (QColor color) \
  { \
    if (color_.to_qcolor () == color) \
      return; \
    color_ = color; \
    Q_EMIT colorChanged (color); \
  } \
  Q_SIGNAL void colorChanged (QColor color); \
  /* ================================================================ */ \
  /* (utils) */ \
  /* ================================================================ */ \
  Q_PROPERTY ( \
    QColor effectiveColor READ getEffectiveColor NOTIFY effectiveColorChanged) \
  QColor getEffectiveColor () const \
  { \
    return get_effective_color (); \
  } \
  Q_SIGNAL void effectiveColorChanged (QColor color);

class ColoredObject
    : virtual public ArrangerObject,
      public zrythm::utils::serialization::ISerializable<ColoredObject>
{
public:
  ColoredObject () = default;
  ~ColoredObject () override = default;
  Q_DISABLE_COPY_MOVE (ColoredObject)

  using Color = zrythm::utils::Color;

  /**
   * @brief Returns the color of the object if set, otherwise the owner track's
   * color.
   */
  QColor get_effective_color () const;

protected:
  void
  copy_members_from (const ColoredObject &other, ObjectCloneType clone_type);

  void init_loaded_base ();

  template <typename Derived> void init_colored_object (this Derived &self)
  {
    const auto update_eff_color = [&self] () {
      Q_EMIT self.effectiveColorChanged (self.get_effective_color ());
    };
    QObject::connect (&self, &Derived::colorChanged, &self, update_eff_color);
    QObject::connect (&self, &Derived::useColorChanged, &self, update_eff_color);
  }

  DECLARE_DEFINE_BASE_FIELDS_METHOD ();

public:
  /**
   * Color independent of owner (Track/Region etc.).
   */
  Color color_;

  /**
   * Whether to use the custom color.
   *
   * If false, the owner color will be used.
   */
  bool use_color_ = false;
};

inline bool
operator== (const ColoredObject &lhs, const ColoredObject &rhs)
{
  return lhs.color_ == rhs.color_ && lhs.use_color_ == rhs.use_color_;
}

#endif // __DSP_COLORED_OBJECT_H__
