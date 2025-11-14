#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


static double buf2nd_order[4];
static double buf_din[8];
static double buf_unw[8];
static double buf_wow[8];
static double buf_flutter[8];


void reset_filters()
{
    memset(buf2nd_order, 0, 4*sizeof(double));
    memset(buf_din, 0, 8*sizeof(double));
    memset(buf_unw, 0, 8*sizeof(double));
    memset(buf_wow, 0, 8*sizeof(double));
    memset(buf_flutter, 0, 8*sizeof(double));
}




double process_2nd_order(register double val)
{
    register double tmp, fir, iir;
    tmp = buf2nd_order[0];
    memmove(buf2nd_order, buf2nd_order + 1, 3 * sizeof(double));
    // use 0.00120740519032883 below for unity gain at 100% level
    iir = val * 0.001207405190260069;
    iir -= 0.9483625336008361 * tmp;
    fir = tmp;
    iir -= -1.73410899821474 * buf2nd_order[0];
    fir += -buf2nd_order[0] - buf2nd_order[0];
    fir += iir;
    tmp = buf2nd_order[1];
    buf2nd_order[1] = iir;
    val = fir;
    iir = val;
    iir -= 0.9533938855978508 * tmp;
    fir = tmp;
    iir -= -1.781298800713404 * buf2nd_order[2];
    fir += buf2nd_order[2] + buf2nd_order[2];
    fir += iir;
    buf2nd_order[3] = iir;
    val = fir;
    return val;
}

double process_DIN(register double val)
{
    register double tmp, fir, iir;
    tmp = buf_din[0];
    memmove(buf_din, buf_din + 1, 7 * sizeof(double));
    // use 9.894850348184627e-007 below for unity gain at 100% level
    iir = val * 9.886712475608222e-007;
    iir -= 0.9718381574433894 * tmp;
    fir = tmp;
    iir -= -1.971551266567659 * buf_din[0];
    fir += -buf_din[0] - buf_din[0];
    fir += iir;
    tmp = buf_din[1];
    buf_din[1] = iir;
    val = fir;
    iir = val;
    iir -= 0.9982440100378892 * tmp;
    fir = tmp;
    iir -= -1.998242909436813 * buf_din[2];
    fir += buf_din[2] + buf_din[2];
    fir += iir;
    tmp = buf_din[3];
    buf_din[3] = iir;
    val = fir;
    iir = val;
    iir -= 0.6434545131997782 * tmp;
    fir = tmp;
    iir -= -1.591050960239724 * buf_din[4];
    fir += buf_din[4] + buf_din[4];
    fir += iir;
    tmp = buf_din[5];
    buf_din[5] = iir;
    val = fir;
    iir = val;
    iir -= 0.9997284329050403 * tmp;
    fir = tmp;
    iir -= -1.999728408318806 * buf_din[6];
    fir += -buf_din[6] - buf_din[6];
    fir += iir;
    buf_din[7] = iir;
    val = fir;
    return val;
}

// Filter descriptions:
//   BpBe4/0.3-200 == Bandpass Bessel filter, order 4, -3.01dB frequencies
//     0.3-200
double process_unweighted(register double val)
{
    register double tmp, fir, iir;
    tmp = buf_unw[0];
    memmove(buf_unw, buf_unw + 1, 7 * sizeof(double));
    // use 0.0003306520826394921 below for unity gain at 100% level
    iir = val * 0.0003306520826380572;
    iir -= 0.6753463035083248 * tmp;
    fir = tmp;
    iir -= -1.591483463373453 * buf_unw[0];
    fir += -buf_unw[0] - buf_unw[0];
    fir += iir;
    tmp = buf_unw[1];
    buf_unw[1] = iir;
    val = fir;
    iir = val;
    iir -= 0.9997682212465883 * tmp;
    fir = tmp;
    iir -= -1.999768186333123 * buf_unw[2];
    fir += -buf_unw[2] - buf_unw[2];
    fir += iir;
    tmp = buf_unw[3];
    buf_unw[3] = iir;
    val = fir;
    iir = val;
    iir -= 0.5771462662841257 * tmp;
    fir = tmp;
    iir -= -1.514102287557188 * buf_unw[4];
    fir += buf_unw[4] + buf_unw[4];
    fir += iir;
    tmp = buf_unw[5];
    buf_unw[5] = iir;
    val = fir;
    iir = val;
    iir -= 0.9995984565721876 * tmp;
    fir = tmp;
    iir -= -1.999598412629212 * buf_unw[6];
    fir += buf_unw[6] + buf_unw[6];
    fir += iir;
    buf_unw[7] = iir;
    val = fir;
    return val;
}

// Filter descriptions:
//   BpBe4/0.3-6 == Bandpass Bessel filter, order 4, -3.01dB frequencies
//     0.3-6
//

// Example code (functionally the same as the above code, but
//  optimised for cleaner compilation to efficient machine code)
double process_wow(register double val)
{

    register double tmp, fir, iir;
    tmp = buf_wow[0];
    memmove(buf_wow, buf_wow + 1, 7 * sizeof(double));
    // use 3.38643522387692e-010 below for unity gain at 100% level
    iir = val * 3.386435216458736e-010;
    iir -= 0.9889822559361133 * tmp;
    fir = tmp;
    iir -= -1.988898714745282 * buf_wow[0];
    fir += -buf_wow[0] - buf_wow[0];
    fir += iir;
    tmp = buf_wow[1];
    buf_wow[1] = iir;
    val = fir;
    iir = val;
    iir -= 0.9997639015233543 * tmp;
    fir = tmp;
    iir -= -1.999763863368945 * buf_wow[2];
    fir += -buf_wow[2] - buf_wow[2];
    fir += iir;
    tmp = buf_wow[3];
    buf_wow[3] = iir;
    val = fir;
    iir = val;
    iir -= 0.9849666019626395 * tmp;
    fir = tmp;
    iir -= -1.984903954482672 * buf_wow[4];
    fir += buf_wow[4] + buf_wow[4];
    fir += iir;
    tmp = buf_wow[5];
    buf_wow[5] = iir;
    val = fir;
    iir = val;
    iir -= 0.9995704510105757 * tmp;
    fir = tmp;
    iir -= -1.999570400238568 * buf_wow[6];
    fir += buf_wow[6] + buf_wow[6];
    fir += iir;
    buf_wow[7] = iir;
    val = fir;
    return val;
}

// Filter descriptions:
//   BpBe4/6-200 == Bandpass Bessel filter, order 4, -3.01dB frequencies
//     6-200
//

// Example code (functionally the same as the above code, but
//  optimised for cleaner compilation to efficient machine code)
double process_flutter(register double val)
{
    register double tmp, fir, iir;
    tmp = buf_flutter[0];
    memmove(buf_flutter, buf_flutter + 1, 7 * sizeof(double));
    // use 0.0002980764585707285 below for unity gain at 100% level
    iir = val * 0.0002980764585582655;
    iir -= 0.6858715731999449 * tmp;
    fir = tmp;
    iir -= -1.605649703918556 * buf_flutter[0];
    fir += -buf_flutter[0] - buf_flutter[0];
    fir += iir;
    tmp = buf_flutter[1];
    buf_flutter[1] = iir;
    val = fir;
    iir = val;
    iir -= 0.9953215690037556 * tmp;
    fir = tmp;
    iir -= -1.995306892110805 * buf_flutter[2];
    fir += -buf_flutter[2] - buf_flutter[2];
    fir += iir;
    tmp = buf_flutter[3];
    buf_flutter[3] = iir;
    val = fir;
    iir = val;
    iir -= 0.5910983651395704 * tmp;
    fir = tmp;
    iir -= -1.532453681510474 * buf_flutter[4];
    fir += buf_flutter[4] + buf_flutter[4];
    fir += iir;
    tmp = buf_flutter[5];
    buf_flutter[5] = iir;
    val = fir;
    iir = val;
    iir -= 0.9916845997627537 * tmp;
    fir = tmp;
    iir -= -1.991665582083071 * buf_flutter[6];
    fir += buf_flutter[6] + buf_flutter[6];
    fir += iir;
    buf_flutter[7] = iir;
    val = fir;
    return val;
}

