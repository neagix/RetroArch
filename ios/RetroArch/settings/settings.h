/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2013 - Jason Fetters
 * 
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

enum SettingTypes
{
   BooleanSetting, ButtonSetting, EnumerationSetting, FileListSetting,
   GroupSetting, AspectSetting, RangeSetting, CustomAction
};

@interface RASettingData : NSObject
@property enum SettingTypes type;

@property (strong) NSString* label;
@property (strong) NSString* name;
@property (strong) NSString* value;

@property (strong) NSString* path;
@property (strong) NSArray* subValues;
@property (strong) NSMutableArray* msubValues;

@property double rangeMin;
@property double rangeMax;

@property bool haveNoneOption;

- (id)initWithType:(enum SettingTypes)aType label:(NSString*)aLabel name:(NSString*)aName;
@end

@interface RAButtonGetter : NSObject<UIAlertViewDelegate>
- (id)initWithSetting:(RASettingData*)setting fromTable:(UITableView*)table;
@end

@interface RASettingEnumerationList : UITableViewController
- (id)initWithSetting:(RASettingData*)setting fromTable:(UITableView*)table;
@end

@interface RASettingsSubList : UITableViewController
- (id)initWithSettings:(NSArray*)values title:(NSString*)title;
- (void)handleCustomAction:(NSString*)action withUserData:(id)data;
- (void)writeSettings:(NSArray*)settingList toConfig:(config_file_t*)config;
@end

@interface RASettingsList : RASettingsSubList
+ (void)refreshModuleConfig:(RAModuleInfo*)module;
- (id)initWithModule:(RAModuleInfo*)module;
@end

@interface RASystemSettingsList : RASettingsSubList
@end