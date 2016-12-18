# FMEncryp-SQLCipher
---      
layout: post      
title:  "SQLCipher 让数据库不再裸奔!"      
date:   2016-07-08 16:09:57 +0800    
categories: jekyll update       
---    
          

# SQLCipher 让数据库不再裸奔

## 一、 数据库加密，不加密，这是个问题

数据存储是基本上每个移动APP都需要使用的，在移动应用平台android/iOS上系统都集成了 **免费版** 的轻量级数据库SQLite，但是，免费版的SQLite有一个致命缺点：**不支持加密**。意味着，你app上的数据库在裸奔！特别是在iOS8.3之前，未越狱的系统也可以通过工具拿到应用程序沙盒里面的文件，后面也有蒸米绕过非越狱手机沙盒读取微信数据库，为了安全考虑，就需要对数据库进行加密。     
        
## 二、 加密方式  
        
#### 对数据库加密的思路有两种：       

1. 将内容加密后再写入数据库   

   这种方式使用简单，在入库/出库只需要将字段做对应的加解密操作即可，一定程度上解决了将数据赤裸裸暴露的问题。但也有很大弊端：     
    * 这种方式并不是彻底的加密，还是可以通过数据库查看到表结构等信息。   
    * 对于数据库的数据，数据都是分散的，要对所有数据都进行加解密操作会严重影响性能。        
  
2. 对数据库文件加密  
      
    将整个数据库整个文件加密，这种方式基本上能解决数据库的信息安全问题。目前已有的SQLite加密基本都是通过这种方式实现的。iOS平台可用的SQLite加密工具：  
    * SQLite Encryption Extension (SEE)   
      事实上SQLite有加解密接口，只是免费版本没有实现而已。而SQLite Encryption Extension (SEE)是SQLite的加密版本，收费的
    * SQLiteEncrypt   
使用AES加密，其原理是实现了开源免费版SQLite没有实现的加密相关接口，SQLiteEncrypt是收费的。
    * SQLiteCrypt   
使用256-bit AES加密，其原理和SQLiteEncrypt一样，都是实现了SQLite的加密相关接口，SQLiteCrypt也是收费的。
    * SQLCipher       
需要说明的是，SQLCipher是完全开源的，代码托管在[Github](https://github.com/sqlcipher/sqlcipher)上。SQLCipher使用256-bit AES加密，由于其基于免费版的SQLite，主要的加密接口和SQLite是相同的，但也增加了一些自己的接口，详情见[官网](https://www.zetetic.net/sqlcipher/)。   
SQLCipher页也分为收费版本和免费版本，官网介绍说收费版只是集成起来更简单，不用再添加OpenSSL依赖库，而且编译速度更快，从功能上来说没有任何区别。


综上：果断选择免费版本SQLCipher作为数据库加密工具。


## 三、 SQLCipher

#### 1. 特性

官网介绍特性：

```
- 快速只有5 - 15%的性能开销加密
- 100%的数据库中的数据文件是加密的
- 使用良好的安全模式(CBC模式,密钥推导)
- 零配置和应用程序级加密
- OpenSSL加密库提供算法     
```
				
#### 2. 特性探究

   * 数据库操作速度   


![](http://7xw5qd.com1.z0.glb.clouddn.com/sqlciper_%E5%9B%BE%E8%A1%A8.png)

	
图(1)、图(2)是官方给出的[Demo](https://github.com/sqlcipher/SQLCipherSpeed)测试出来的，图(1)中数据表没有设置索引，图(2)中数据表设置了索引，图(3)在官方Demo中增加的一组多线程下处理事务的测试


   * 加密解密过程     
   
	   SQLite数据库设计中考虑了安全问题并预留了加密相关的接口。但是并没有给出实现。SQLite 数据库源码中通过使用SQLITE_HAS_CODEC宏来控制是否使用数据库加密。并且预留了四个结构让用户自己实现以达到对数据库进行加密的效果。这四个接口分别是：     
	sqlite3_key()： 指定数据库使用的密钥    
	sqlite3_rekey()：为数据库重新设定密钥；   
	sqlite3CodecGetKey()：返回数据库的当前密钥     
	sqlite3CodecAttach()： 将密钥及页面编码函数与数据库进行关联。  
	 
	 而sqlcipher就是实现这四个接口以及自己的一些接口，从源码入手看一下加解密过程：
  
  定位创建数据库时设置数据库密码的函数： 
  
```
- (BOOL)setKeyWithData:(NSData *)keyData {
	 #ifdef SQLITE_HAS_CODEC
	 if (!keyData) {
	     return NO;
	 }
	 int rc = sqlite3_key(_db, [keyData bytes], (int)  [keyData length]);
	 return (rc == SQLITE_OK);
	 #else
	 #pragma unused(keyData)
	 return NO;
	 #endif
}
 
```
	
查找sqlite3_key( ),进入sqlite3.c文件，找到sqlite3CodecAttach，这个函数内部做了几个重要的事情：   

1. 初始化sqlcipher，以及在sqlcipher_openssl_setup中设置加密算法:

	sqlcipher_openssl_random：盐值 (randnum)  
	sqlcipher_openssl_kdf：hash （sha1）    
	sqlcipher_openssl_cipher：aes (aes-256)    

2. sqlcipher_codec_ctx_init,最核心的东西就是这个函数调用,将上层传进来的初始密码与数据库的指针通过一个codec_ctx的结构体绑定起来了，codec_ctx的结构定义如下：   

	```
	struct codec_ctx {
	  int kdf_salt_sz;
	  int page_sz;
	  unsigned char *kdf_salt;
	  unsigned char *hmac_kdf_salt;
	  unsigned char *buffer;
	  Btree *pBt;
	  cipher_ctx *read_ctx;
	  cipher_ctx *write_ctx;
	  unsigned int skip_read_hmac;
	  unsigned int need_kdf_salt;
	};
	```
这个codec_ctx实际上就类似于每次打开sqlcipher数据库后，可以对数据库操作的句柄，在数据库未关闭之前，一直在维护在内存中，为后续的增删改查操作使用。其中的ciper_ctx比较关键，就是它持有了我们输入的原始密码。 

3. sqlite3pager_sqlite3PagerSetCodec,将密钥及页面编码函数与数据库进行关联

接下来看看密码是怎样被处理，既然知道算法指针kdf、cipher以及初始密码pass指针，全局搜索”->kdf、->cipher、->pass”查找所有使用到访问这些指针的地方，定位到sqlite3Codec便是处理密钥推导、加密数据库内容的地方。注意到sqlcipher_cipher_ctx_key_derive和sqlcipher_page_cipher函数：

```
void* sqlite3Codec(void *iCtx, void *data, Pgno pgno, int mode) {
...
  //call to derive keys if not present yet
  if((rc = sqlcipher_codec_key_derive(ctx)) != SQLITE_OK) {
   sqlcipher_codec_ctx_set_error(ctx, rc); 
   return NULL;
  }
...

...
      if(pgno == 1) memcpy(buffer, SQLITE_FILE_HEADER, FILE_HEADER_SZ);       //copy file header to the first 16 bytes of the page 
      
      
      rc = sqlcipher_page_cipher(ctx, CIPHER_READ_CTX, pgno, CIPHER_DECRYPT, page_sz - offset, pData + offset, (unsigned char*)buffer + offset);
..

}	
从注释看来，sqlcipher_codec_key_derive推导密钥，作用是实现原始密码到真正的加密KEY的转化，即HASH加盐以及AES；sqlcipher_page_cipher则是对数据页加密，看来找到加密的关键入口了。
```

由此可以总结出数据库加解密的核心逻辑，解密的操作是以page为单位进行的(每一个page就是db文件的每1024个字节），流程如下：

![](http://7xw5qd.com1.z0.glb.clouddn.com/sqlciper_%E6%B5%81%E7%A8%8B%E5%9B%BE.png)

#### 3. 集成

* 途径： 
  1. 如果你使用cocoapod的话就不需要自己配置了，为了方便，我们直接使用FMDB进行操作数据库，FMDB也支持SQLCipher  ：       
     pod ‘FMDB/SQLCipher’     
  2. 做依赖工程引入，详见[官网](https://www.zetetic.net/sqlcipher/ios-tutorial/)，有详细图文说明        
* 使用：     
   1. 重写FMDatabase里面的**- (BOOL)open**和**- (BOOL)openWithFlags:(int)flags vfs:(NSString *)vfsName** 方法，增加设置密码的操作[self setKey:kDBencryptKey]，其中的kDBencryptKey为数据库密钥，通过写一个类目来实现，减小对现有代码的入侵，减低耦合。
   2. 对app中已经存在的数据库，需要先对对数据库进行加密，实现一个数据库加密解密工具类。

编写的类目和工具类地址：[github](https://github.com/muhuashanjin/FMEncryp-SQLCipher.git)

   

