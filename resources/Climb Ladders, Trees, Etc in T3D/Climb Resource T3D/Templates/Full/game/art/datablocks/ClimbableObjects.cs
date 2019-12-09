//Climb Resource
datablock StaticShapeData(Ladder)
{
   // Mission editor category, this datablock will show up in the
   // specified category under the "shapes" root category.
   category = "ClimableObjects";

   // Basic Item properties
   shapefile = "~/shapes/ladders/ladder.dts";
   
   //The following line is what makes this object climable, it dosent have to be a ladder, could be a tree, wall, etc.
   dynamicType = $TypeMasks::ClimableItemObjectType;
   mass = 1;
   friction = 2;
   elasticity = 0.3;
};