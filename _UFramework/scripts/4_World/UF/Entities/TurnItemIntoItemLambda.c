modded class TurnItemIntoItemLambda extends ReplaceItemWithNewLambda
{

	override void CopyOldPropertiesToNew (notnull EntityAI old_item, EntityAI new_item)
	{
		super.CopyOldPropertiesToNew(old_item, new_item);

		if (new_item && old_item) 
		{
			if (ItemBase.Cast(new_item) && ItemBase.Cast(old_item))
			{
				ItemBase.Cast(new_item).SetTexture(ItemBase.Cast(old_item).GetCurrentSkinIdx())
			}
		}
	}
}