#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "bin2hex.h"

#define MAX_HEX_LINE_LENGTH 64 // Maximum length allowed for a hex line
typedef enum
{
    iHex_data            = 0x00 ,
    iHex_endOfFile       = 0x01 ,
    iHex_ExtendedSegAddr = 0x02 ,
    iHex_StartSegAddr    = 0x03 ,
    iHex_ExtLinearAddr   = 0x04 ,
    iHex_StartLinearAddr = 0x05
} iHexType_t ;

uint32_t getFileSize( FILE * in ) ;
uint32_t intelHexLine( uint8_t size , uint16_t addr , iHexType_t type , uint8_t * data , char * outPut ) ;

bool bin2hex(const char* binary_file, const char* hex_file, uint32_t start_address)
{
    FILE * fileIn  = NULL ;
    FILE * fileOut = NULL ;
    uint32_t fileInSize , fileOutSize , i ;
    int chr , ret ;

    uint8_t buffer[ 16 ] ;
    char bufferOut[ MAX_HEX_LINE_LENGTH ] ;
    uint8_t bufferIndex ;
    uint32_t addrCount ;
    
    // Initial messages.
    printf( "Bin To Intel Hex \n");

    // Open Binary File.
    fileIn = fopen( binary_file , "rb" ) ;
    if( fileIn == ( ( FILE * ) NULL ) )
    {
        printf( "\tError to open \"%s\"\n" , binary_file ) ;
        return( false ) ;
    }

    // Check binary file size.
    fileInSize = getFileSize( fileIn ) ;
    printf( "\t\"%s\" - Size = %lu\n" , binary_file , ( unsigned long ) fileInSize ) ;
    
    // Create Intel Hex File.
    fileOut = fopen( hex_file , "wb" ) ;
    if( fileOut == ( ( FILE * ) NULL ) )
    {
        fclose( fileIn ) ;
        printf( "\tError to create \"%s\"\n" , hex_file ) ;
        return( false ) ;
    }
    
    // Echo Extended Linear Address Record
    buffer[ 0 ] = ( uint8_t ) ( start_address >> 24 ) ;
    buffer[ 1 ] = ( uint8_t ) ( start_address >> 16 ) ;
    ( void ) intelHexLine( 2 , 0x0000 , iHex_ExtendedSegAddr , buffer , bufferOut ) ;
    fprintf( fileOut , "%s\n" , bufferOut ) ;
    for( i = 0 , fileOutSize = 0 , bufferIndex = 0 , addrCount = start_address ; i < fileInSize ; i++ )
    {
        chr = getc( fileIn ) ;
        if( chr == EOF )
        {
            ret = ferror( fileIn ) ;
            printf( "\tError to read binary file at %lu!\n" , ( unsigned long ) i ) ;
            printf( "\tferror [ %d ]\n" , ret ) ;
            fclose( fileIn ) ;
            fclose( fileOut ) ;
            return( ret ) ;
        }
        
        buffer[ bufferIndex++ ] = ( uint8_t ) chr ;
        
        if( bufferIndex >= 16 )
        {
            ( void ) intelHexLine( bufferIndex , ( uint16_t ) addrCount , iHex_data , buffer , bufferOut ) ;
            fileOutSize += ( uint32_t ) fprintf( fileOut , "%s\n" , bufferOut ) ;
            addrCount += bufferIndex ;
            bufferIndex = 0 ;
            
            // Already turn 64KBytes.
            if( ( addrCount & 0x0000FFFF ) == 0x00000000 )
            {
                buffer[ 0 ] = ( uint8_t ) ( addrCount >> 24 ) ;
                buffer[ 1 ] = ( uint8_t ) ( addrCount >> 16 ) ;
                ( void ) intelHexLine( 2 , 0x0000 , iHex_ExtendedSegAddr , buffer , bufferOut ) ;
                fileOutSize += ( uint32_t ) fprintf( fileOut , "%s\n" , bufferOut ) ;
            }
        }
    }

    if( bufferIndex )
    {
        ( void ) intelHexLine( bufferIndex , addrCount , iHex_data , buffer , bufferOut ) ;
        fileOutSize += ( uint32_t ) fprintf( fileOut , "%s\n" , bufferOut ) ;
    }

    ( void ) intelHexLine( 0x00 , 0x0000 , iHex_endOfFile , NULL , bufferOut ) ;
    fileOutSize += ( uint32_t ) fprintf( fileOut , "%s\n" , bufferOut ) ;
    
    printf( "\t\"%s\" - Size = %lu\n" , hex_file , ( unsigned long ) fileOutSize ) ;
	printf( "\tDone!\n" ) ;

    fclose( fileIn ) ;
    fclose( fileOut ) ;
    
    return(true) ;
}

uint32_t getFileSize( FILE * in )
{
    uint32_t ret = 0 ;
    fpos_t pos ;
    int fPosRet ;
    
    fPosRet = fgetpos( in , &pos ) ;
    if( fPosRet )
    {
        return( ret ) ;
    }
    
    fPosRet = fseek( in , 0 , SEEK_END ) ;
    if( fPosRet )
    {
        return( ret ) ;
    }
    
    ret = ( uint32_t ) ftell( in ) ;
    if( ret == 0xFFFFFFFF )
    {
        ret = 0 ;
        return( ret ) ;
    }

    ( void ) fsetpos( in , &pos ) ;
    
    return( ret ) ;
}

uint32_t intelHexLine( uint8_t size , uint16_t addr , iHexType_t type , uint8_t * data , char * outPut )
{
    int outIndex = 0 ;
    uint8_t cks ;
            
    // Start Code + Byte Count.
    outIndex = snprintf( outPut , 4, ":%02X" , size ) ;
    cks = size ;
            
    // Address.
    outIndex += snprintf( outPut + outIndex , 5 , "%04X" , addr ) ;
    cks += ( uint8_t ) ( addr >> 8 ) ;
    cks += ( uint8_t ) ( addr      ) ;
    
    // Record type.
    outIndex += snprintf( outPut + outIndex , 3 , "%02X" , ( uint8_t ) type ) ;
    cks += ( uint8_t ) type ;
            
    // Data.
    for( int i = 0 ; ( i < size ) && ( data != NULL ) ; i++ )
    {
        outIndex += snprintf( outPut + outIndex , 3 , "%02X" , data[ i ] ) ;
        cks += data[ i ] ;
    }
    
    // Two's complement.
    cks  = ~cks ;
    cks += 1 ;

    // Check sum.
    outIndex += snprintf( outPut + outIndex , 3 ,  "%02X" , cks ) ;

    return( ( uint32_t ) outIndex ) ;
}