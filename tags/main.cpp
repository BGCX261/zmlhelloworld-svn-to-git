/*
-------------------------------------------------------
这是一个模拟人类记忆的容器.他会像人的遗忘
曲线一样删除容器内的内容,每隔一段时间,容器会自动
清理一次.

从list继承而来的,记忆容器只能输入一次.这样





-------------------------------------------------------
*/






//=====================class random======================

//	因为他使用time( 0 )来设置种子
//	这个函数不是标准C++头文件里面的函数.
//
//
//	_initseed 是初始化种子,在第一次使用时
//	_initseed会设置为true.这样一个程序,只
//	会设置一次种子.这个部分是不需要手动去
//	完成的.
//
//
//	使用这个类有三种方法:
//	方法一:
//	random rand;
//	rand();
//	推荐的用法.效率最高;
//	
//	方法二:
//	random();
//	直接返回随机数,方便的调用;
//
//	方法三:
//	random()();
//	怪异的不推荐的用法;	



#include <cstdlib>
#include <ctime>

class random{
public:
	random();

	int operator()(){ return ::rand(); }
	operator int(){ return ::rand(); }
private:
	static bool _initseed;
};

bool random::_initseed = false;

random::random()
{
	if( !_initseed )
	{
		::srand( ( unsigned int )time( 0 ) );
		_initseed = true;
	}
}

//========================================================






//============================timer class=============================

#include <windows.h>

class Timer{
public:
	Timer()
	{
		if( !_setFreq )
		{
			_setFreq = true;
			QueryPerformanceFrequency( (LARGE_INTEGER*)&_Freq );
		}
		start();
	}
	bool reset()
	{
		return start();
	}

	bool start()
	{
		return QueryPerformanceCounter( (LARGE_INTEGER*)&_start );
	}
	bool end()
	{
		return QueryPerformanceCounter( (LARGE_INTEGER*)&_end );
	}

	LONGLONG operator()()
	{
		end();
		return ( _end - _start ) * 1000000 / _Freq;
	}
private:
	LONGLONG _start;
	LONGLONG _end;
	static LONGLONG _Freq;
	static bool _setFreq;  
};

bool Timer::_setFreq = false;
LONGLONG Timer::_Freq = 0;

//======================================================================



/*===============================================================================================================
=================================================================================================================
===============================================================================================================*/




#include <list>
using namespace std;


/*============================================================================
|				模拟人类记忆的容器元件
|		继承自std::list,该容器一旦输入数据完成之后,就无法再改动
|	数据将被锁定.
|		如果要输入新的信息,需要使用新的容器.这么做的是为了将各个
|	时间段输入的数据分开,混在一起将不好处理.
|		
|		人类的遗忘曲线类似双曲线,方程是[ y = m / x ], x为逝去的时间
|	y为应该剩下的信息, m是一个常数.
|		所以这里把信息的删除用双曲线来处理.
|	
|	***这个类将作为外部容器的元件来是使用,这是重新评估其地位的结果***
|	
==============================================================================*/
template <typename Type>
class human_memory_item : public list<Type>{

public:
	human_memory_item()
		:_lock( false ){}						//	类对象初始化时解锁

	template <typename Iter>
	human_memory_item( Iter first, Iter last )	//	另一种基类构造函数的引入
		:list<Type>::list( first, last ),
		_lock( false )
	{}




	//human_memory_item& forget();				//	一个临时测试用的接口,已废弃,不要使用.

	human_memory_item& lock();					//	锁定接口,锁定的时候表示数据输入完成,锁定时
	//	自动保存_orig_size,_orig_time,并设置_lock
	human_memory_item& forget();				//	遗忘接口,根据逝去的时间自动删除


	//protected:

	int _get_del_num( clock_t time );			//	根据逝去的时间长度得到删除的数目

	human_memory_item& _del( int del_num );		//	删除条目的内部接口.

	int _orig_size;								//	用于记录初始大小以计算该删除多少条目
	bool _lock;									//	判断是否锁定,
	clock_t _orig_time;							//	锁定时的的时间,用来做为以后删除多少条目的依据.
};



/*-----------------------------------------------------------
|		删除接口,随机删除mun个条目
|		
|		该接口使用的是没有经过优化的最普通的
|	原始算法,以后需要优化,被注释起来的部分
|	效率很好,但是不能随机的删除.
|
|		没有被注释起来的部分,能随机的删除指定
|	的条目,但是不效率很低,需要改善.
|	
-------------------------------------------------------------*/
template <typename Type>
human_memory_item<Type>& human_memory_item<Type>::_del( int del_num )	//	参数:请求删除的条目数量
{
	if( del_num >= (int)size() )		//	判定: 如果请求的条目数量大于或者等于已有条目的数量
		clear();						//		那么清空容器.
	if( del_num <= 0 || size() == 0 )	//	判定: 如果请求的条目数量小于或者等于0, 或者容器内的条目数量为0 
		return *this;					//		直接返回,不做任何处理.


	/*--------------------------------------------
	|		该算法会已适当的间隔来删除容器内
	|	的条目.
	|	但是会出现一个严重的问题:
	|		就当请求删除的条目数量大于容器已有
	|	条目数量的一半的时候,算出来的步进大小会
	|	小于1,于是会出现只删除前N项的情况.
	|		这样得到的结果是只留下容器中后半部
	|	分的条目,这是很不理想的.相比下面的方法
	|	他的效率要高,但是基于随机性的问题,暂时不
	|	使用.
	---------------------------------------------*/
	//int step = size() / del_num;						//	step为步进的大小

	//for( list<Type>::iterator it = begin();		
	//	it != end(); )
	//{
	//	if( del_num <= 0 )							//	如果请求删除的条目数量小于或者等于0,与下面的num--对应,以达到
	//		break;									//		删除完指定数目的条目后就跳出,而不多删.
	//	erase( it++ );								//	删除当前迭代器位置的条目,使用it++,是因为当如果使用it,删除后
	//												//		迭代器会会停在已删除的位置,所以需要++,不然迭代器无法前进.
	//	del_num--;										//	删除一个之后,请求条目数量减1.
	//	for( int i = 0; i < step - 1; i++ )			//	该循环使迭代器向前移动step-1次,减1次是对于之前的erase( it++ )
	//	{				
	//		if( it == end() )						//	如果it == end()就退出,在这里加一个判断是因为,当it在这个循环内
	//			break;								//	的时候,无法以上级循环中的条件来判断.
	//		it++;									//	it先前进1.
	//	}
	//}

	/*--------------------------------------------
	|	该算法是常规的算法:
	|		每次删除先得到一个小于容器大小size()
	|	的随机数,然后将迭代器迭代到随机数指定的
	|	位置,进行删除.
	---------------------------------------------*/
	for( int i = 0; i < del_num; i++ )				
	{		
		int	r = random() % size();					//	得到一个小于容器大小size()的随机数;
		list<Type>::iterator it = begin();			
		for( int k = 0; k < r; k++ )				//	将迭代器迭代到随机数指定的位置
			it++;
		list::erase( it );							//	删除条目
	}

	return *this; 
}



///*---------------------------------------------------------
//|		一个临时测试用的接口,废弃
//---------------------------------------------------------*/
//template <typename Type>
//human_memory_item<Type>& human_memory_item<Type>::forget()
//{
//	if( !_lock )
//	{
//		_origsize = size();
//		_lock = true;
//	}
//	for( int i = 1; i <= _orig_size; i++ )
//	{
//		int num = size()  - _orig_size / i * 2;// - _orig_size * 0.1;
//
//		_del( num );
//
//		for( list<Type>::iterator it = begin();
//			it != end(); it++ )
//			wcout << *it ;//<< ' ';
//		wcout << '[' << size() << ']' << endl;
//		
//	}
//	
//	return *this;
//}



/*---------------------------------------------------------
|		根据逝去的时间来得到需要删除条目的多少
|	单独把这个过程做成一个函数,是为了能够方便修改
|	其中的各个参数,以适用于实际的情况.这是内联函数
---------------------------------------------------------*/
template <typename Type>
inline int human_memory_item<Type>::_get_del_num( clock_t time )	//	参数:逝去的时间
{
	if( !time )
		return 0;
	return	size() - _orig_size / time;
}					
//	首先,根据公式计算应该保留的条目. y = m / x
//	y为保留的条目数,x为时间.那么y = m / time;
//	然后用现成的条目得到应该要删除的条目:
//		size() - y => size - m / time;
//	m 无法确定,暂时定为_orig_size; <<====该常数需要修改



/*----------------------------------------------------------
|		锁定接口,锁定的时候表示数据输入完成,锁定时
|	自动保存_orig_size,_orig_time,并设置_lock
----------------------------------------------------------*/
template <typename Type>
human_memory_item<Type>& human_memory_item<Type>::lock()
{
	_orig_time = clock();		
	_orig_size = size();
	_lock = true;
	return *this;
}

/*----------------------------------------------------------
|	遗忘接口,根据逝去的时间自动删除
----------------------------------------------------------*/
template <typename Type>
human_memory_item<Type>& human_memory_item<Type>::forget()				
{
	_del( _get_del_num( ( clock() - _orig_time ) / 1000 ) );	//	 clock() - _orig_time 为逝去的时间,单位秒

	return *this;
}



/*===========================================================================
|						模拟人类记忆的容器
|		以以上的容器元件为内部构成,该容器主要依靠std::list为内部数据
|	来管理元件.
|
|		需要提供以下接口:
|	1,输入数据:输入一次后,会使用一个元件.之后这个元件被锁定
|	2,显示数据:显示所有数据
|	3,搜索数据:搜素数据,并显示相关数据
|
|	4,自动清理用的内部接口:用于遍历整个容器,清理每个元件的forget
|	和删除size()为0的空元件;
===========================================================================*/
template <typename Type>									//	模板参数Type:是为了确定元件中的数据类型
class human_memory{
public:
	template <typename Iter>								//	从外部Type类型容器和数组获得数据
	human_memory& learn( Iter first, Iter last );			//	参数,外部容器或数组的迭代器.

	human_memory& recall( Type key );						//	根据输入的关键词搜索数据(回忆);


	human_memory& show_all();								//	显示所有数据;输出到控制台一般调试用

protected:
	human_memory& _clear();									//	内部的清理数据接口.负责整理容器内所有数据.						


	list<human_memory_item<Type>*> _human_memory;			//	以以上的容器元件为元素的list容器作为,这里使用指针
	//	是为了避免大型对象的复制造成的
};


/*----------------------------------------------------------
|		输入数据的接口.
|	输入的数据是元件的Type类型的容器或数组.
|		***注意***
|	此接口使用了new.需要在删除元件之前delete
----------------------------------------------------------*/
template <typename Type> template <typename Iter>
human_memory<Type>& human_memory<Type>::learn( Iter first, Iter last )			//	参数:外部容器或数组的迭代器.
{
	human_memory_item<Type>* p_hmi = new human_memory_item<Type>( first, last);	//	new内存来装来自外部的数据.
	//	***注意这里使用了new,在删除元
	//	件的时候必须要先使用delete

	p_hmi->lock();																//	锁定元件.

	_human_memory.push_back( p_hmi );											//	把元件装到记忆中.数据输入完毕

	return *this;
}


/*----------------------------------------------------------
|		自动清理用的内部接口
|	用于遍历整个容器,使每个元件forget()
|	和删除size()为0的空元件;
----------------------------------------------------------*/
template <typename Type>
human_memory<Type>& human_memory<Type>::_clear()
{
	/*-------------------------------------
	|	for遍历,暂不使用
	-------------------------------------*/
	//for( list<human_memory_item<Type>*>::iterator it = _human_memory.begin();
	//	it != _human_memory.end(); it++ )											//	遍历所有的元件
	//{
	//	if(	( *it )->empty() )														//	如果元件为空,就删除.
	//	{
	//		delete *it;																//	对应输入数据接口中的new
	//		_human_memory.erase( it++ );											//	it++是为了删除后it能继续遍历
	//	}
	//	else
	//		( *it )->forget();														//	调用元件中的遗忘接口

	//	if( it == _human_memory.end() )
	//		break;
	//}

	/*------------------------------------
	|	while遍历,逻辑上更合理些
	------------------------------------*/
	list<human_memory_item<Type>*>::iterator it = _human_memory.begin();
	while( it != _human_memory.end() )												//	如果迭代器到末尾就退出循环
	{
		if(	( *it )->empty() )														//	如果元件为空,就删除.
		{
			delete *it;																//	对应输入数据接口中的new
			_human_memory.erase( it++ );											//	it++是为了删除后it能继续遍历
		}
		else
		{
			( *it )->forget();														//	调用元件中的遗忘接口
			it++;
		}
	}

	return *this;
}


/*----------------------------------------------------------
|	2,显示数据:显示所有数据,cout输出
----------------------------------------------------------*/
template <typename Type>
human_memory<Type>& human_memory<Type>::show_all()
{
	_clear();																		//	自动整理一次

	for( list<human_memory_item<Type>*>::iterator it = _human_memory.begin();
		it != _human_memory.end(); it++ )											//	遍历所有的元件
	{
		for( human_memory_item<Type>::iterator item_it = ( *it )->begin();			//	遍历每一个元件中
			item_it != ( *it )->end(); item_it++ )									//	的每一个条目
			cout << *item_it << ' ';												//	并输出到控制台

		cout << " [" << ( clock() - ( *it )->_orig_time ) / 1000 << "]\n";
		//	显示元件锁定后逝去的时间
	}

	return *this;
}



/*-----------------------------------------------------------
|	根据输入的关键词搜索数据(回忆);
-----------------------------------------------------------*/
template <typename Type>
human_memory<Type>& human_memory<Type>::recall( Type key )				    		//	根据输入的关键词搜索数据(回忆);
{
	_clear();																		//	自动整理一次

	for( list<human_memory_item<Type>*>::iterator it = _human_memory.begin();
		it != _human_memory.end(); it++ )											//	遍历所有的元件
	{
		bool found = false;
		for( human_memory_item<Type>::iterator item_it = ( *it )->begin();			//	遍历每一个元件中
			item_it != ( *it )->end(); item_it++ )									//	的每一个条目
			if(	*item_it == key )													//	如果条目中含有关键字
			{
				found = true;														//	标记为找到
				break;																//	停止遍历
			}
			if( found )
			{
				cout << "found:";
				for( human_memory_item<Type>::iterator item_it = ( *it )->begin();		//	遍历每一个元件中
					item_it != ( *it )->end(); item_it++ )								//	的每一个条目
					cout << *item_it << ' ';											//	输出到控制台


				cout << " [" << ( clock() - ( *it )->_orig_time ) / 1000 << "]\n";
				//	显示元件锁定后逝去的时间
			}
	}

	return *this;
}




#include <iostream>
#include <list>
#include <string>




int main()
{


	////const int dsize = 500;
	////human_memory_item<int> data;
	////for( int i = 0; i < dsize; i++ )
	////	data.push_back( i );

	//////Timer t;
	//////data.forget( 60 );
	//////cout << t() << '\n';

	//////for( list<int>::iterator it = data.begin();
	//////	it != data.end(); it++ )
	//////	cout << *it << ' ';

	//////cout << '\n' << data.size();



	//////for( int i = 1; i <= dsize; i++ )
	//////{
	//////	int num = data.size()  - dsize / i * 2;// - dsize * 0.1;

	//////	data.forget( num );

	//////	for( list<int>::iterator it = data.begin();
	//////		it != data.end(); it++ )
	//////		cout << *it << ' ';
	//////	cout << '[' << data.size() << ']' << endl;
	//////	
	//////}
	//////


	////data.forget();

	//std::wcout.imbue(std::locale("chs"));   //   设置locale，否则也不行...   

	//typedef list< human_memory_item<wchar_t> > memorys;

	//memorys brain;

	//human_memory_item_item<wchar_t> news;

	//wstring str = L"对泰国稍微有所了解的人一定知道，这个被誉为“微笑之地”的国家有一座“天使之城”，那就是它的首都曼谷。而这几天，在这个95%的居民都信仰佛教的国家，在曼谷这个“美玉的宝库，无法征服之地、轮回转世神灵的庇护地和居住地”，身穿绿色迷彩服的军人和身穿红色衣衫的示威者们却开了“杀戒”。";

	//for( int i = 0; i < str.size(); i++ )
	//	news.push_back( str[ i ] );

	//news.forget();

	//brain.push_back( news );


	//human_memory<string> brain;

	//for( int i = 0; i < 10; i++ )
	//{
	//	list<string> str;

	//	str.push_back( "You" );
	//	str.push_back( "just" );
	//	str.push_back( "invalidated" );
	//	str.push_back( "your" );
	//	str.push_back( "iterator" );
	//	str.push_back( "by" );
	//	str.push_back( "calling" );
	//	str.push_back( "erase" );

	//	brain.learn( str.begin(), str.end() );
	//	Sleep( 1000 );
	//}

	//while( true )
	//	brain.show_all();

	//brain.recall( "You" );



	//int size = 0x6000000;



	//while( true )
	//{

	//	int* pi = new int[ size ];
	//	int* pi1 = new int[ size ];
	//	Sleep( 1000 );
	//	delete [] pi1;
	//	Sleep( 1000 );
	//	delete [] pi;
	//	Sleep( 1000 );
	//	int* pi2 = new int[ size ];

	//	Sleep( 20000 );
	//	//size *= 1.2;
	//	delete [] pi2;
	//	Sleep( 1000 );

	//}


	//Timer t;					//	设置计时器
	//
	//	cout << "test";			//	为了忽略第一次使用cout的消耗;

	//t.reset();					//	重置时间

	//	cout << "dld" << "d" << endl;

	//cout << '[' << t() << ']';	//	显示逝去的时间
	//t.reset();
	//
	//	cout << "dld";
	//
	//cout << '[' << t() << ']'; 
	//t.reset();

	//	cout << "d" << endl;

	//cout << '[' << t() << ']';


	human_memory<int> test;
	test.human_memory<int>::human_memory();


	return 0;
}