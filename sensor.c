/*
 * The code is released under the GNU General Public License.
 * Developed by Gilles Lasnier; based on a first implementation by Mark Williams
 * Created 25th March 2014
 * Working for the AltIMU-10 Pololu board
 */

#include "i2c-dev.h"
#include "L3G.h"
#include "LSM303.h"
#include "LPS331.h"

int file;

/*
 *
 */
void  readBlock(uint8_t command, uint8_t size, uint8_t *data)
{
    int result = i2c_smbus_read_i2c_block_data(file, command, size, data);
    if (result != size)
    {
        printf("Failed to read block from I2C.");
        exit(1);
    }
}

/*
 *
 */
void selectDevice(int file, int addr)
{
        char device[3];
        if (addr == 1)
        {
                device == "L3G";
        }
        else if (addr == 2) {
                device == "LSM";
        }
        else {
                device == "LPS";
        }

        if (ioctl(file, I2C_SLAVE, addr) < 0) {
                fprintf(stderr,
                        "Error: Could not select device  0x%02x: %s\n",
                        device, strerror(errno));
        }
}

/*
 *
 */
void readACC(int *a)
{
 	uint8_t block[6];
    selectDevice(file, ACC_ADDRESS);
	readBlock(0x80 | LSM303_OUT_X_L_A, sizeof(block), block);

    *a = (int16_t)(block[0] | block[1] << 8) >> 4;
    *(a+1) = (int16_t)(block[2] | block[3] << 8) >> 4;
    *(a+2) = (int16_t)(block[4] | block[5] << 8) >> 4;
}

/*
 *
 */
void readMAG(int *m)
{
	uint8_t block[6];
    selectDevice(file, MAG_ADDRESS);
	// DLHC: register address order is X,Z,Y with high bytes first
	readBlock(0x80 | LSM303_OUT_X_H_M, sizeof(block), block);

    *m = (int16_t)(block[1] | block[0] << 8);
    *(m+1) = (int16_t)(block[5] | block[4] << 8) ;
    *(m+2) = (int16_t)(block[3] | block[2] << 8) ;
}

/*
 *
 */
void readGYR(int *g)
{
	uint8_t block[6];
    selectDevice(file, GYR_ADDRESS);
	readBlock(0x80 | L3G_OUT_X_L, sizeof(block), block);

    *g = (int16_t)(block[1] << 8 | block[0]);
    *(g+1) = (int16_t)(block[3] << 8 | block[2]);
    *(g+2) = (int16_t)(block[5] << 8 | block[4]);
}

/*
 *
 */
void print_bytes(void *p, size_t len)
{
    size_t i;
    printf("(");
    for (i = 0; i < len; ++i)
        printf("%02X", ((unsigned char*)p)[i]);
    printf(")");
}

/*
 *
 */
void readPRE(int *p)
{
	uint8_t block[3];
    selectDevice(file, PRE_ADDRESS);
	readBlock(LPS331_PRESS_OUT_XL | (1 << 7), sizeof(block), block);
    
    *p = (int32_t)(int8_t)block[2] << 16 | (uint16_t)block[1] << 8 | block[0];
}

/*
 *
 */
void readTEM(int *t)
{
	uint8_t block[2];
    selectDevice(file, PRE_ADDRESS);
	readBlock(LPS331_TEMP_OUT_L | (1 << 7), sizeof(block), block);
    
    *t = (int16_t)(block[1] << 8 | block[0]);
}

/*
 *
 */
void writeAccReg(uint8_t reg, uint8_t value)
{
    selectDevice(file, ACC_ADDRESS);
    int result = i2c_smbus_write_byte_data(file, reg, value);
    if (result == -1)
    {
        printf ("Failed to write byte to I2C Acc.");
        exit(1);
    }
}

/*
 *
 */
void writeMagReg(uint8_t reg, uint8_t value)
{
    selectDevice(file, MAG_ADDRESS);
    int result = i2c_smbus_write_byte_data(file, reg, value);
    if (result == -1)
    {
        printf("Failed to write byte to I2C Mag.");
        exit(1);
    }
}

/*
 *
 */
void writeGyrReg(uint8_t reg, uint8_t value)
{
    selectDevice(file, GYR_ADDRESS);
    int result = i2c_smbus_write_byte_data(file, reg, value);
    if (result == -1)
    {
        printf("Failed to write byte to I2C Gyr.");
        exit(1);
    }
}

/*
 *
 */
void writePreReg(uint8_t reg, uint8_t value)
{
    selectDevice(file, PRE_ADDRESS);
    int result = i2c_smbus_write_byte_data(file, reg, value);
    if (result == -1)
    {
        printf("Failed to write byte to I2C Pre.");
        exit(1);
    }
}

/*
 *
 */
void enableIMU()
{
	__u16 block[I2C_SMBUS_BLOCK_MAX];

    int res, bus, size;

    char filename[20];
    sprintf(filename, "/dev/i2c-%d", 1);
    
    file = open(filename, O_RDWR);
    if (file<0) {
		printf("Unable to open I2C bus!");
        exit(1);
    }

    // Enable accelerometer
    
    // z,y,x axis enabled , 100Hz data rate
	writeAccReg(LSM303_CTRL_REG1_A, 0b01010111);
    
    // +/- 8G full scale: FS = 10 on DLHC, high resolution output mode
    writeAccReg(LSM303_CTRL_REG4_A, 0b00101000);
 
    // Enable magnetometer
    writeMagReg(LSM303_MR_REG_M, 0x00);

    // Enable gyro
    
    // Normal power mode, all axes enabled
    writeGyrReg(L3G_CTRL_REG1, 0b00001111);
    
    // Continuous update, 2000 dps full scale
    writeGyrReg(L3G_CTRL_REG4, 0b00110000);
    
    // Enable altimeter (pressure + barometer)
    
    // Active mode, 12.5 Hz output data rate
    writePreReg(LPS331_CTRL_REG1, 0b11100000);
}

