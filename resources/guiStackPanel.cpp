//-----------------------------------------------------------------------------
// TORQUE 3D
// Copyright (C) GarageGames.com, Inc.
//
// GuiStackPanel: Enhancement of StackCrtl done by by Elvince
//-----------------------------------------------------------------------------

#include "GuiStackPanel.h"

IMPLEMENT_CONOBJECT(GuiStackPanel);

static const EnumTable::Enums stackTypeEnum[] =
{
   { GuiStackPanel::stackingTypeVert, "Vertical"  },
   { GuiStackPanel::stackingTypeHoriz,"Horizontal" },
   { GuiStackPanel::stackingTypeDyn,"Dynamic" }
};
static const EnumTable gStackTypeTable(3, &stackTypeEnum[0]);

static const EnumTable::Enums stackHorizEnum[] =
{
   { GuiStackPanel::horizStackLeft, "Left to Right"  },
   { GuiStackPanel::horizStackLeftLeftward, "Left to Right Leftward"  },
   { GuiStackPanel::horizStackRight,"Right to Left" },
   { GuiStackPanel::horizStackRightLeftward,"Right to Left LeftWard" }
};
static const EnumTable gStackHorizSizingTable(4, &stackHorizEnum[0]);

static const EnumTable::Enums stackVertEnum[] =
{
   { GuiStackPanel::vertStackTop, "Top to Bottom"  },
   { GuiStackPanel::vertStackTopUpward,"Top to Bottom Upward" },
   { GuiStackPanel::vertStackBottom,"Bottom to Top" },
	{ GuiStackPanel::vertStackBottomUpward,"Bottom to Top Upward" }
	
};
static const EnumTable gStackVertSizingTable(4, &stackVertEnum[0]);

GuiStackPanel::GuiStackPanel()
{
   setMinExtent(Point2I(16,16));
   mMaxExtent=Point2I(0,0);
   mResizing = false;
   mStackingType = stackingTypeVert;
   mStackVertSizing = vertStackTop;
   mStackHorizSizing = horizStackLeft;
   mPadding = 0;
   mIsContainer = true;
   mDynamicSize = true;
   mChangeChildSizeToFit = true;
   mChangeChildPosition = true;
   mExtensionSize=getWidth();
}

void GuiStackPanel::initPersistFields()
{

   addGroup( "Stacking" );
   addField( "StackingType",           TypeEnum,   Offset(mStackingType, GuiStackPanel), 1, &gStackTypeTable);
   addField( "HorizStacking",          TypeEnum,   Offset(mStackHorizSizing, GuiStackPanel), 1, &gStackHorizSizingTable);
   addField( "VertStacking",           TypeEnum,   Offset(mStackVertSizing, GuiStackPanel), 1, &gStackVertSizingTable);
   addField( "Padding",                TypeS32,    Offset(mPadding, GuiStackPanel));
   addField( "DynamicSize",            TypeBool,   Offset(mDynamicSize, GuiStackPanel));
   addField( "ChangeChildSizeToFit",   TypeBool,   Offset(mChangeChildSizeToFit, GuiStackPanel));
   addField( "ChangeChildPosition",    TypeBool,   Offset(mChangeChildPosition, GuiStackPanel));
   addField( "MaxExtent",			   TypePoint2I , Offset(mMaxExtent, GuiStackPanel));
   addField( "ExtensionSize",         TypeS32,    Offset(mExtensionSize, GuiStackPanel));
   endGroup( "Stacking" );

   Parent::initPersistFields();
}

ConsoleMethod( GuiStackPanel, freeze, void, 3, 3, "%stackCtrl.freeze(bool) - Prevents control from restacking")
{
   object->freeze(dAtob(argv[2]));
}

ConsoleMethod( GuiStackPanel, updateStack, void, 2, 2, "%stackCtrl.updateStack() - Restacks controls it owns")
{
   object->updatePanes();
}

bool GuiStackPanel::onWake()
{
   if ( !Parent::onWake() )
      return false;

   updatePanes();

   return true;
}

void GuiStackPanel::onSleep()
{
   Parent::onSleep();
}

void GuiStackPanel::updatePanes()
{
   // Prevent recursion
   if(mResizing) 
      return;

   // Set Resizing.
   mResizing = true;

   Point2I extent = getExtent();

   // Do we need to stack horizontally?
   if( ( extent.x > extent.y && mStackingType == stackingTypeDyn ) || mStackingType == stackingTypeHoriz )
   {
      switch( mStackHorizSizing )
      {
      case horizStackLeft:
         stackFromLeft();
         break;
      case horizStackRight:
         stackFromRight();
         break;
	case horizStackLeftLeftward:
         stackFromLeftLeftward();
         break;
      case horizStackRightLeftward:
         stackFromRightLeftward();
         break;
      }
   }
   // Or, vertically?
   else if( ( extent.y > extent.x && mStackingType == stackingTypeDyn ) || mStackingType == stackingTypeVert)
   {
      switch( mStackVertSizing )
      {
      case vertStackTop:
         stackFromTop();
         break;
      case vertStackBottom:
         stackFromBottom();
         break;
	  case vertStackBottomUpward:
		  stackFromBottomUpward();
		  break;		
	  case vertStackTopUpward:
		 stackFromTopUpward();
		  break;
      }
   }

   // Clear Sizing Flag.
   mResizing = false;
}

void GuiStackPanel::freeze(bool _shouldfreeze)
{
   mResizing = _shouldfreeze;
}

void GuiStackPanel::stackFromBottom()
{
   Point2I curPos = Point2I(0,0);
   S32 ColumnOffSet=0;
   S32 MaxWidth=0;
   Point2I newExtent= Point2I(0,0);
   // Now work up from there!
   for(S32 i=size()-1; i>=0; i--)
   {
      // Place control
      GuiControl * gc = dynamic_cast<GuiControl*>(operator [](i));
      if ( gc && gc->isVisible() )
	  {
		 Point2I childPos = curPos;
         if ( !mChangeChildPosition )
            childPos.x = gc->getLeft();
		 else
		 {
			 if ((curPos.y + gc->getHeight() > mMaxExtent.y) && mMaxExtent.y>0) 
			 {//We need to create a new column
				 ColumnOffSet+=MaxWidth + mPadding;
				 if (newExtent.y < curPos.y)
					newExtent.y = curPos.y;
				 curPos.y=0;
				 childPos.y = 0;
				 MaxWidth=0;
			 }
			 //todo:if <0 then offset the objposition of max width

			 childPos.x +=ColumnOffSet;
		 }
         // Make it have our width but keep its height, if appropriate
         if ( mChangeChildSizeToFit )
         {
            gc->resize(childPos, Point2I(mExtensionSize, gc->getHeight()));		
         }
         else
         {
            gc->resize(childPos, Point2I(gc->getWidth(), gc->getHeight()));				
         }
		if (MaxWidth<gc->getWidth())
			MaxWidth=gc->getWidth();
         // Update our state...
		 curPos.y += gc->getHeight() + ((i < size()-1) ? mPadding : 0);       
      }
   }
   if (newExtent.y < curPos.y)
		newExtent.y = curPos.y;
   if ( mDynamicSize )
   {
       // Conform our size to the sum of the child sizes.
       newExtent.x = ColumnOffSet+MaxWidth;//getWidth();
       newExtent.y = getMax( newExtent.y, getMinExtent().y );
       resize( getPosition(), newExtent );
   }
}
void GuiStackPanel::stackFromBottomUpward()
{
   // If we go from the top, except that item are rendered in reverse ordered
   // Upward will extent your control from the top and not the bottom as usual

   Point2I curPos = Point2I(0,0);
   S32 ColumnOffSet=0;
   S32 MaxWidth=0;
   Point2I newExtent= Point2I(0,0);
   // Figure the final position
   Point2I basePos = getPosition() + Point2I(0,getHeight());
   
   for(S32 i=size()-1; i>=0; i--)
   {
      // Place control
      GuiControl * gc = dynamic_cast<GuiControl*>(operator [](i));
      if ( gc && gc->isVisible() )
      {
		 Point2I childPos = curPos;
         if ( !mChangeChildPosition )
            childPos.x = gc->getLeft();
		 else
		 {
			 if ((curPos.y + gc->getHeight() > mMaxExtent.y) && mMaxExtent.y>0) 
			 {//We need to create a new column
				 ColumnOffSet+=MaxWidth + mPadding;
				 if (newExtent.y < curPos.y)
					newExtent.y = curPos.y;
				 curPos.y=0;
				 childPos.y = 0;
				 MaxWidth=0;
			 }
			//todo:if <0 then offset the objposition of max width

			 childPos.x +=ColumnOffSet;
		 }
         // Make it have our width but keep its height, if appropriate
         if ( mChangeChildSizeToFit )
         {
            gc->resize(childPos, Point2I(mExtensionSize, gc->getHeight()));
         }
         else
         {
            gc->resize(childPos, Point2I(gc->getWidth(), gc->getHeight()));
         }
		 if (MaxWidth<gc->getWidth())
			MaxWidth=gc->getWidth();
         // Update our state...
		 curPos.y += gc->getHeight() + ((i > 0) ? mPadding : 0);       
      }
   }
   if (newExtent.y < curPos.y)
		newExtent.y = curPos.y;
   if ( mDynamicSize )
   {
       // Conform our size to the sum of the child sizes.
       newExtent.x = ColumnOffSet+MaxWidth;//getWidth();
       newExtent.y = getMax( newExtent.y, getMinExtent().y );
       resize( Point2I(basePos.x,basePos.y-newExtent.y) , newExtent );
   }
}

void GuiStackPanel::stackFromTop()
{
   Point2I curPos = Point2I(0,0);
   S32 ColumnOffSet=0;
   S32 MaxWidth=0;
   Point2I newExtent= Point2I(0,0);
   // Now work up from there!
   for (S32 i = 0; i < size(); i++)
   {
      // Place control
      GuiControl * gc = dynamic_cast<GuiControl*>(operator [](i));
      if ( gc && gc->isVisible() )
	  {
		 Point2I childPos = curPos;
         if ( !mChangeChildPosition )
            childPos.x = gc->getLeft();
		  else
		 {
			 if ((curPos.y + gc->getHeight() > mMaxExtent.y) && mMaxExtent.y>0) 
			 {//We need to create a new column
				 ColumnOffSet+=MaxWidth + mPadding;
				 if (newExtent.y < curPos.y)
					newExtent.y = curPos.y;
				 curPos.y=0;
				 childPos.y = 0;
				 MaxWidth=0;
			 }
			 //todo:if <0 then offset the objposition of max width

			 childPos.x +=ColumnOffSet;
		 }
         // Make it have our width but keep its height, if appropriate
         if ( mChangeChildSizeToFit )
         {
            gc->resize(childPos, Point2I(mExtensionSize, gc->getHeight()));
         }
         else
         {
            gc->resize(childPos, Point2I(gc->getWidth(), gc->getHeight()));
         }
		 if (MaxWidth<gc->getWidth())
			MaxWidth=gc->getWidth();
         // Update our state...
		 curPos.y += gc->getHeight() + ((i < size()-1) ? mPadding : 0);       
      }
   }
   if (newExtent.y < curPos.y)
		newExtent.y = curPos.y;
   if ( mDynamicSize )
   {
       // Conform our size to the sum of the child sizes.
       newExtent.x = ColumnOffSet+MaxWidth;//getWidth();
       newExtent.y = getMax( newExtent.y, getMinExtent().y );
       resize( getPosition(), newExtent );
   }
}

void GuiStackPanel::stackFromTopUpward()
{
   // Upward will extent your control from the top and not the bottom as usual

   // Figure out where we're starting...
   Point2I curPos = Point2I(0,0);
   // Figure the final position
   Point2I basePos = getPosition() + Point2I(0,getHeight());
   
   for (S32 i = 0; i < size(); i++)
   {
      // Place control
      GuiControl * gc = dynamic_cast<GuiControl*>(operator [](i));
      if ( gc && gc->isVisible() )
      {
		 Point2I childPos = curPos;
         if ( !mChangeChildPosition )
            childPos.x = gc->getLeft();
         // Make it have our width but keep its height, if appropriate
         if ( mChangeChildSizeToFit )
         {
            gc->resize(childPos, Point2I(getWidth(), gc->getHeight()));
         }
         else
         {
            gc->resize(childPos, Point2I(gc->getWidth(), gc->getHeight()));
         }
         // Update our state...
		 curPos.y += gc->getHeight() + ((i > 0) ? mPadding : 0);       
      }
   }
   if (mDynamicSize)
   {
       // Conform our size to the sum of the child sizes.
       curPos.x = getWidth();
       curPos.y = getMax( curPos.y, getMinExtent().y );
	   resize( Point2I(basePos.x,basePos.y-curPos.y) , curPos );
   }
}
void GuiStackPanel::stackFromLeft()
{
   // Position and resize everything...
   Point2I curPos = Point2I(0, 0);
   S32 RowOffSet=0;
   S32 MaxHeight=0;
   Point2I newExtent= Point2I(0,0);
   // Place each child...
   for (S32 i = 0; i < size(); i++)
   {
      // Place control
      GuiControl * gc = dynamic_cast<GuiControl*>(operator [](i));
      if ( gc && gc->isVisible() )
      {
         Point2I childPos = curPos;
         if ( !mChangeChildPosition )
            childPos.y = gc->getTop();
		else
		 {
			 if ((curPos.x + gc->getWidth() > mMaxExtent.x) && mMaxExtent.x>0) 
			 {//We need to create a new row
				 RowOffSet+=MaxHeight + mPadding;
				 if (newExtent.x < curPos.x)
					newExtent.x = curPos.x;
				 curPos.x=0;
				 childPos.x = 0;
				 MaxHeight=0;
			 }
			 //todo:if <0 then offset the objposition of max width

			 childPos.y +=RowOffSet;
		 }
         // Make it have our height but keep its width, if appropriate
         if ( mChangeChildSizeToFit )
         {
            gc->resize(childPos, Point2I( gc->getWidth(),mExtensionSize ));
         }
         else
         {
            gc->resize(childPos, Point2I( gc->getWidth(), gc->getHeight() ));
         }
		if (MaxHeight<gc->getHeight())
			MaxHeight=gc->getHeight();
         // Update our state...
         curPos.x += gc->getWidth() + ((i < size() - 1) ? mPadding : 0);
      }
   }
	if (newExtent.x < curPos.x)
		newExtent.x = curPos.x;
   if ( mDynamicSize )
   {
       // Conform our size to the sum of the child sizes.
       newExtent.x = getMax( newExtent.x, getMinExtent().x );
       newExtent.y = RowOffSet+MaxHeight;
       resize( getPosition(), newExtent );
   }
}
void GuiStackPanel::stackFromLeftLeftward()
{
	// Leftward will extent your control from the left and not the right as usual
   // Position and resize everything...
   Point2I curPos = Point2I(0, 0);
	// Figure the final position
   Point2I basePos = getPosition() + Point2I(getWidth(),0);

   // Place each child...
   for (S32 i = 0; i < size(); i++)
   {
      // Place control
      GuiControl * gc = dynamic_cast<GuiControl*>(operator [](i));
      if ( gc && gc->isVisible() )
      {
         Point2I childPos = curPos;
         if ( !mChangeChildPosition )
            childPos.y = gc->getTop();

         // Make it have our height but keep its width, if appropriate
         if ( mChangeChildSizeToFit )
         {
            gc->resize(childPos, Point2I( gc->getWidth(), getHeight() ));
         }
         else
         {
            gc->resize(childPos, Point2I( gc->getWidth(), gc->getHeight() ));
         }

         // Update our state...
         curPos.x += gc->getWidth() + ((i < size() - 1) ? mPadding : 0);
      }
   }

   if ( mDynamicSize )
   {
       // Conform our size to the sum of the child sizes.
       curPos.x = getMax( curPos.x, getMinExtent().x );
       curPos.y = getHeight();
       resize( Point2I(basePos.x-curPos.x,basePos.y), curPos );
   }
}
void GuiStackPanel::stackFromRight()
{
	// Position and resize everything...
   Point2I curPos = Point2I(0, 0);
   S32 RowOffSet=0;
   S32 MaxHeight=0;
   Point2I newExtent= Point2I(0,0);
   // Place each child...
   for(S32 i=size()-1; i>=0; i--)
   {
      // Place control
      GuiControl * gc = dynamic_cast<GuiControl*>(operator [](i));
      if ( gc && gc->isVisible() )
      {
         Point2I childPos = curPos;
         if ( !mChangeChildPosition )
            childPos.y = gc->getTop();
		 else
		 {
			 if ((curPos.x + gc->getWidth() > mMaxExtent.x) && mMaxExtent.x>0) 
			 {//We need to create a new row
				 RowOffSet+=MaxHeight + mPadding;
				 if (newExtent.x < curPos.x)
					newExtent.x = curPos.x;
				 curPos.x=0;
				 childPos.x = 0;
				 MaxHeight=0;
			 }
			//todo:if <0 then offset the objposition of max width

			 childPos.y +=RowOffSet;
		 }
         // Make it have our height but keep its width, if appropriate
         if ( mChangeChildSizeToFit )
         {
            gc->resize(childPos, Point2I( gc->getWidth(),mExtensionSize ));
         }
         else
         {
            gc->resize(childPos, Point2I( gc->getWidth(), gc->getHeight() ));
         }
		if (MaxHeight<gc->getHeight())
			MaxHeight=gc->getHeight();
         // Update our state...
         curPos.x += gc->getWidth() + ((i > 0) ? mPadding : 0);
      }
   }
   if (newExtent.x < curPos.x)
		newExtent.x = curPos.x;
   if ( mDynamicSize )
   {
       // Conform our size to the sum of the child sizes.
        newExtent.x = getMax( newExtent.x, getMinExtent().x );
       newExtent.y = RowOffSet+MaxHeight;
       resize( getPosition(), newExtent );
   }
}
void GuiStackPanel::stackFromRightLeftward()
{// Leftward will extent your control from the left and not the right as usual
	// Position and resize everything...
   Point2I curPos = Point2I(0, 0);
// Figure the final position
   Point2I basePos = getPosition() + Point2I(getWidth(),0);

   // Place each child...
   for(S32 i=size()-1; i>=0; i--)
   {
      // Place control
      GuiControl * gc = dynamic_cast<GuiControl*>(operator [](i));
      if ( gc && gc->isVisible() )
      {
         Point2I childPos = curPos;
         if ( !mChangeChildPosition )
            childPos.y = gc->getTop();

         // Make it have our height but keep its width, if appropriate
         if ( mChangeChildSizeToFit )
         {
            gc->resize(childPos, Point2I( gc->getWidth(), getHeight() ));
         }
         else
         {
            gc->resize(childPos, Point2I( gc->getWidth(), gc->getHeight() ));
         }

         // Update our state...
         curPos.x += gc->getWidth() + ((i > 0) ? mPadding : 0);
      }
   }

   if ( mDynamicSize )
   {
       // Conform our size to the sum of the child sizes.
       curPos.x = getMax( curPos.x, getMinExtent().x );
       curPos.y = getHeight();
       resize( Point2I(basePos.x-curPos.x,basePos.y), curPos );
   }
}
bool GuiStackPanel::resize(const Point2I &newPosition, const Point2I &newExtent)
{
   if( !Parent::resize( newPosition, newExtent ) )
      return false;

   updatePanes();

   // CodeReview This logic should be updated to correctly return true/false
   //  based on whether it sized it's children. [7/1/2007 justind]
   return true;
}

void GuiStackPanel::addObject(SimObject *obj)
{
   Parent::addObject(obj);

   updatePanes();
}

void GuiStackPanel::removeObject(SimObject *obj)
{
   Parent::removeObject(obj);

   updatePanes();
}

bool GuiStackPanel::reOrder(SimObject* obj, SimObject* target)
{
   bool ret = Parent::reOrder(obj, target);
   if (ret)
      updatePanes();

   return ret;
}

void GuiStackPanel::childResized(GuiControl *child)
{
   updatePanes();
}