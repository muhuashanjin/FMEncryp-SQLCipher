//
//  FMEncryptHelper.h
//  EKESell
//
//  Created by thy on 16/6/28.
//
//

#import <Foundation/Foundation.h>

@interface FMEncryptHelper : NSObject

+ (BOOL)encryptDatabase:(NSString *)path encryptKey:(NSString *)encryptKey;

+ (BOOL)unEncryptDatabase:(NSString *)path encryptKey:(NSString *)encryptKey;

+ (BOOL)encryptDatabase:(NSString *)sourcePath targetPath:(NSString *)targetPath encryptKey:(NSString *)encryptKey;

+ (BOOL)unEncryptDatabase:(NSString *)sourcePath targetPath:(NSString *)targetPath encryptKey:(NSString *)encryptKey;

+ (BOOL)changeKey:(NSString *)dbPath originKey:(NSString *)originKey newKey:(NSString *)newKey;

@end
