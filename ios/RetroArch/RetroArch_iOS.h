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

#import <UIKit/UIKit.h>

@interface RetroArch_iOS : UINavigationController<UIApplicationDelegate, UINavigationControllerDelegate>

+ (void)displayErrorMessage:(NSString*)message;

+ (RetroArch_iOS*)get;

- (void)runGame:(NSString*)path withModule:(RAModuleInfo*)module;
- (void)refreshConfig;
- (void)refreshSystemConfig;

@property (strong, nonatomic) NSString* systemConfigPath;

@property (strong, nonatomic) NSString* system_directory;

@end
