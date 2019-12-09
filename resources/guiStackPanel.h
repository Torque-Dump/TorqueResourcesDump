//-----------------------------------------------------------------------------
// TORQUE 3D
// Copyright (C) GarageGames.com, Inc.
//
// GuiStackPanel: Enhancement of StackCrtl done by by Elvince
//-----------------------------------------------------------------------------

#ifndef _GUISTACKCTRL_H_
#define _GUISTACKCTRL_H_

#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif

#include "gfx/gfxDevice.h"
#include "console/console.h"
#include "console/consoleTypes.h"

/// A stack of GUI controls.
///
/// This maintains a horizontal or vertical stack of GUI controls. If one is deleted, or
/// resized, then the stack is resized to fit. The order of the stack is
/// determined by the internal order of the children (ie, order of addition).
///
///
class GuiStackPanel : public GuiControl
{
protected:
   typedef GuiControl Parent;
   bool  mResizing;
   S32   mPadding;
   S32   mStackHorizSizing;      ///< Set from horizSizingOptions.
   S32   mStackVertSizing;       ///< Set from vertSizingOptions.
   S32   mStackingType;
   bool  mDynamicSize;           ///< Resize this control to fit the size of the children (width or height depends on the stack type)
   bool  mChangeChildSizeToFit;  ///< Does the child resize to fit i.e. should a horizontal stack resize its children's height to fit?
   bool  mChangeChildPosition;   ///< Do we reset the child's position in the opposite direction we are stacking?
   Point2I mMaxExtent;
   S32   mExtensionSize;

public:
   GuiStackPanel();

   enum stackingOptions
   {
      horizStackLeft = 0,///< Stack from left to right when horizontal
      horizStackRight,   ///< Stack from right to left when horizontal
      vertStackTop,      ///< Stack from top to bottom when vertical
      vertStackBottom,   ///< Stack from bottom to top when vertical
      stackingTypeVert,  ///< Always stack vertically
      stackingTypeHoriz, ///< Always stack horizontally
      stackingTypeDyn ,   ///< Dynamically switch based on width/height
	  vertStackBottomUpward, ///< Stack from bottom to top when vertical and extent upward
	  vertStackTopUpward, ///< Stack from top to bottom when vertical and extent upward
	  horizStackLeftLeftward,///< Stack from left to right when horizontal and extent leftward
      horizStackRightLeftward   ///< Stack from right to left when horizontal and extent leftward
   };


   bool resize(const Point2I &newPosition, const Point2I &newExtent);
   void childResized(GuiControl *child);
   /// prevent resizing. useful when adding many items.
   void freeze(bool);

   bool onWake();
   void onSleep();

   void updatePanes();

   virtual void stackFromLeft();
	virtual void stackFromLeftLeftward();
   virtual void stackFromRight();
   virtual void stackFromRightLeftward();
   virtual void stackFromTop();
   virtual void stackFromTopUpward();
   virtual void stackFromBottom();
   virtual void stackFromBottomUpward();

   S32 getCount() { return size(); }; /// Returns the number of children in the stack

   void addObject(SimObject *obj);
   void removeObject(SimObject *obj);

   bool reOrder(SimObject* obj, SimObject* target = 0);

   static void initPersistFields();
   
   DECLARE_CONOBJECT(GuiStackPanel);
   DECLARE_CATEGORY( "Gui Containers" );
   DECLARE_DESCRIPTION( "A container controls that arranges its children in a vertical or\n"
                        "horizontal stack." );
};

#endif