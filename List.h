
#pragma once

#ifdef _DEBUG
#define DBG(_x) _x
#else
#define DBG(_x)
#endif

template <class T> class List_c
{
public:
	List_c() :
	  m_pList( NULL ),
	  m_iCount( 0 ),
	  m_iTotalSize( 0 ),
	  m_bAllocate( true ),
	  m_iItemSize( sizeof( T ) )
	{
	}

	List_c( const List_c &other )
	{
		m_bAllocate = other.m_bAllocate;

		if ( m_bAllocate )
		{
			m_pList = NULL;
			m_iCount = 0;
			m_iTotalSize = 0;
			m_iItemSize = sizeof( T );

			for ( int i=0; i<other.m_iCount; i++ )
				Append( other.at( i ) );
		}
		else
		{
			m_pList = other.m_pList;
			m_iCount = other.m_iCount;
			m_iTotalSize = other.m_iTotalSize;
			m_iItemSize = other.m_iItemSize;
		}
	}

	~List_c()
	{
		Clear();
	}

	void UseExistingList( void *pExistingList, int iCount, int iItemSize )
	{
		Clear();

		m_bAllocate = false;
		m_pList = pExistingList;
		m_iCount = iCount;
		m_iItemSize = iItemSize;
		m_iTotalSize = m_iCount * m_iItemSize;
	}

	List_c &operator = ( const List_c &other )
	{
		Clear();

		m_bAllocate = other.m_bAllocate;

		if ( m_bAllocate )
		{
			for ( int i=0; i<other.m_iCount; i++ )
				Append( other.at( i ) );
		}
		else
		{
			m_pList = other.m_pList;
			m_iCount = other.m_iCount;
			m_iTotalSize = other.m_iTotalSize;
		}

		return *this;
	}

	List_c &operator += ( const List_c &other )
	{
		for (int i = 0; i < other.m_iCount; i++)
			Append(other[i]);

		return *this;
	}

	List_c &operator += ( const List_c *other )
	{
		for (int i = 0; i < other->m_iCount; i++)
			Append((*other)[i]);

		return *this;
	}

	List_c &operator += ( T value )
	{
		Append(value);

		return *this;
	}

	inline int Count() const
	{
		return m_iCount;
	}

	inline const T &at( int iIndex ) const
	{
		DBG( ATASSERT( iIndex >= 0 ) );
		DBG( ATASSERT( iIndex < m_iCount ) );

		return *(T*)( m_pList + iIndex * m_iItemSize );
	}

	inline T &operator[] ( int iIndex )
	{
		DBG( ATASSERT( iIndex >= 0 ) );
		DBG( ATASSERT( iIndex < m_iCount ) );

		return *(T*)( m_pList + iIndex * m_iItemSize );
	}

	inline void Append( T value )
	{
		Allocate( m_iCount + 1 );

		T *t = new ( m_pList + m_iCount * m_iItemSize ) T( value );

		m_iCount++;
	}

	inline void Append (const List_c &other)
	{
		for (int i = 0; i < other.m_iCount; i++)
			Append(other[i]);
	}

	inline T *AppendNewInstance()
	{
		Allocate( m_iCount + 1 );

		T *t = new ( m_pList + m_iCount * m_iItemSize ) T();

		m_iCount++;

		return t;
	}

	inline void Reserve( int iItemCount )
	{
		Allocate( iItemCount );
	}

	inline int Find( T value ) const
	{
		// TODO: implement proper search
		for ( int i=0; i<m_iCount; i++ )
		{
			if ( value == *(T*)( m_pList + i * m_iItemSize ) )
			{
				return i;
			}
		}

		return -1; // Not found.
	}

	inline void Sort()
	{
		Sort( 0, m_iCount );
	}

	inline void RemoveAt( int iIndex )
	{
		DBG( ATASSERT( iIndex >= 0 ) );
		DBG( ATASSERT( iIndex < m_iCount ) );

		if ( iIndex < 0 || iIndex >= m_iCount ) return;

		for ( int i=iIndex; i<m_iCount-1; i++ )
		{
			*(T*)( m_pList + i * m_iItemSize ) = *(T*)( m_pList + ( i + 1 ) * m_iItemSize );
		}

		m_iCount--;
	}

	void Clear()
	{
		if ( m_pList )
		{
			if ( m_bAllocate )
			{
				for ( int i=0; i<m_iCount; i++ )
				{
					( (T*)( m_pList + i * m_iItemSize ) )->~T();
				}

				free( m_pList );
			}

			m_pList = NULL;
			m_iCount = 0;
			m_iTotalSize = 0;
		}
	}

	inline void Swap( int i1, int i2 )
	{
		DBG( ATASSERT( m_bAllocate ) ); // llemarie: can only sort lists allocated by this class.

		// TODO llemarie: This is potentially slow.
		T tmp( *(T*)( m_pList + i1 * m_iItemSize ) );
		*(T*)( m_pList + i1 * m_iItemSize ) = *(T*)( m_pList + i2 * m_iItemSize );
		*(T*)( m_pList + i2 * m_iItemSize ) = tmp;
	}

protected:

	static const int m_iTotalSizeIncrement = 200;

	void Allocate( int iCount )
	{
		if ( ! m_bAllocate )
		{
			m_iTotalSize = iCount;
			return;
		}

		if ( iCount > m_iTotalSize )
		{
			do
			{
				m_iTotalSize += m_iTotalSizeIncrement;
			}
			while ( iCount > m_iTotalSize );

			if ( m_pList )
			{
				// TODO llemarie: Slow, creates a copy of each instance of T then frees the olf memory
				DBG( static int iListAllocateCount = 0 );
				//DBG( qDebug( "WARNING: %s Use of slow list reallocation (%s items) in %s\n", format_number( iListAllocateCount++ ), format_number( m_iCount ), __FUNCTION__ ) );
				unsigned char *pOldList = m_pList;
				m_pList = (unsigned char *)malloc( m_iTotalSize * m_iItemSize );
				for ( int i=0; i<m_iCount; i++ )
				{
					new ( m_pList + i * m_iItemSize ) T( *(T*)( pOldList + i * m_iItemSize ) );
					( (T*)( pOldList + i * m_iItemSize ) )->~T();
				}
				free( pOldList );
			}
			else
			{
				m_pList = (unsigned char *)malloc( m_iTotalSize * m_iItemSize );
			}
		}
	}

	void Sort( int iStart, int iEnd )
	{
		DBG( ATASSERT( m_bAllocate ) ); // llemarie: can only sort lists allocated by this class.

		while ( 1 )
		{
			int iSpan = iEnd - iStart;
			if ( iSpan < 2 ) return;

			iEnd--;
			int iLow = iStart;
			int iHigh = iEnd - 1;
			int iPivot = iStart + iSpan / 2;

			if ( *(T*)( m_pList + iEnd   * m_iItemSize ) < *(T*)( m_pList + iStart * m_iItemSize ) ) Swap( iEnd, iStart );
			if ( iSpan == 2 ) return;

			if ( *(T*)( m_pList + iPivot * m_iItemSize ) < *(T*)( m_pList + iStart * m_iItemSize ) ) Swap( iPivot, iStart);
			if ( *(T*)( m_pList + iEnd   * m_iItemSize ) < *(T*)( m_pList + iPivot * m_iItemSize ) ) Swap( iEnd  , iPivot);
			if ( iSpan == 3 ) return;

			Swap( iPivot, iEnd );

			while ( iLow < iHigh )
			{
				while ( iLow < iHigh && *(T*)( m_pList + iLow * m_iItemSize ) < *(T*)( m_pList + iEnd * m_iItemSize ) )
					iLow++;

				while ( iHigh > iLow && *(T*)( m_pList + iEnd * m_iItemSize ) < *(T*)( m_pList + iHigh * m_iItemSize ) )
					iHigh--;

				if ( iLow < iHigh )
				{
					Swap( iLow, iHigh );
					iLow++;
					iHigh--;
				}
				else
				{
					break;
				}
			}

			if ( *(T*)( m_pList + iLow * m_iItemSize ) < *(T*)( m_pList + iEnd * m_iItemSize ) )
				iLow++;

			Swap( iEnd, iLow );
			Sort( iStart, iLow );

			iStart = iLow + 1;
			iEnd++;
		}
	}

	unsigned char *m_pList;
	int  m_iCount;
	int  m_iTotalSize;
	bool m_bAllocate;
	int  m_iItemSize;
};

