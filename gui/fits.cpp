#include <CCfits/CCfits>
#include <QDateTime>
#include <QDebug>
#include "fits.h"
#include "mddasdatapoint.h"
#include "mddasplotconfig.h"

using namespace CCfits;
using namespace std;

/* Save MDDAS acquired data in a FITS file */
int writeFitsImage(const QVector<MDDASDataPoint>* data_vec, 
                   const QVector<double>* ts_vec, 
                   const QDateTime timestamp, 
                   const double exp_time, 
                   const MDDASPlotConfig *det_cfg,
                   const QMap<QString, QVariant> *_settings) {

    /* be quiet! except for debugging...*/
    FITS::setVerboseMode(true);

    qDebug() << "Preparing to save fits";

    /* Fail if there's no data to save */
    if (data_vec->size() <= 0)
        return -1;

    
    /* Number of axes */
    long naxis = 2;
    /* Size of axes Note: naxes[0] = y, naxes[1] = x*/
    long naxes[2] = { det_cfg->getYMax(), det_cfg->getXMax()  };
    /* number of rows for the table! */
    long tableRows(data_vec->size());

    qDebug() << "Table rosw: " << tableRows;

    string plhduname("PHOTON_LIST");

    std::valarray<unsigned int> xArray(tableRows);
    std::valarray<unsigned int> yArray(tableRows);
    std::valarray<unsigned int> pArray(tableRows);
    std::valarray<double> tArray(tableRows);

    std::vector<string> colName(4, "");
    std::vector<string> colForm(4, "");
    std::vector<string> colUnit(4, "");

    colName[0] = "X";
    colName[1] = "Y";
    colName[2] = "PHD";
    colName[3] = "TIME";

    colForm[0] = "1J";
    colForm[1] = "1J";
    colForm[2] = "1J";
    colForm[3] = "1D";

    colUnit[0] = "pixels";
    colUnit[1] = "pixels";
    colUnit[2] = "pixels";
    colUnit[3] = "seconds";

    std::string fname((timestamp.toString("yyyy-MM-dd-hhmmss")).toUtf8().constData());
    fname = std::string(fname) + ".fits";

    std::string fits_date((timestamp.toString("dd.MM.yyyy")).toUtf8().constData());

    qDebug() << "Creating file pointer";

    /* declare auto-pointer to FITS at function scope. Ensures no resources
       leaked if something fails in dynamic allocation. */
    std::auto_ptr<FITS> pFits(0);
    // C++11 newness
    //std::unique_ptr<FITS> pFits(0);

    try {
        /* overwrite existing file if the file already exists. */        
        const std::string fileName(fname);
        
        /* Create a new FITS object, specifying the data type and axes
           for the primary image. Simultaneously create the
           corresponding file. */
        
        if (_settings->value("fitsImage").toBool()) {
            /* ULONG_IMG: each pixel can hold a 32 bit unsigned value */
            pFits.reset( new FITS(fileName , ULONG_IMG , naxis , naxes ) );
        } else {
            //pFits.reset( new FITS(fileName , ULONG_IMG , naxis , naxes ) );
            pFits.reset( new FITS(fileName, CCfits::Write, false, std::vector<String>() ));
        }
    } catch (const FITS::CantCreate &cc) {
        /* ... or not, as the case may be. */
        std::cerr << cc.message() << std::endl;
        return -1;       
    }
    
    //pFits->setCompressionType(GZIP_1);

    /* references for clarity. */
    /* Recall -- naxes[0] is the X axis... */
    long& numberOfRows = naxes[0];
    long nelements(1); 
    
    /* number of pixels */
    nelements = naxes[0]*naxes[1];

    //std::valarray<unsigned long> pixelArray(nelements);
    std::valarray<unsigned long> pixelArray(0);
    if (_settings->value("fitsImage").toBool()) {
        /* array to hold pixel values */
        pixelArray.resize(nelements);
        //std::valarray<unsigned long> pixelArray(nelements);
    }

    /* Hold the index of the current timestamp key */
    //uint t_index = 0;

    qDebug() << "Filling binary table arrays";

    /* Put all of the data into the image. Fill binary table arrays
       with data. */
    for (long i = 0; i < data_vec->size(); i++) {
        if (_settings->value("fitsImage").toBool()) {
            pixelArray[data_vec->value(i).y()*numberOfRows + data_vec->value(i).x()] += 1;
        }
        xArray[i] = (unsigned int)data_vec->value(i).x();
        yArray[i] = (unsigned int)data_vec->value(i).y();
        pArray[i] = (unsigned int)data_vec->value(i).p();
        /* Time is handled by timestamp per group of indices. Each key
           in the timestamp map is the index after which the timestamp
           value is applied until the next index. */
        //tArray[i] = ts_vec->value((ts_vec->keys())[t_index]);
        tArray[i] = ts_vec->value(i);

        // Old way of doing the time was slow...
        // if ((t_index + 1) < ts_vec->keys().size()) {
        //     if (i > (ts_vec->keys())[t_index + 1]) {
        //         t_index++;
        //     }
        // }
    }
     
    /* FITS starts with pixel 1 for whatever reason */
    long fpixel(1);

    if (_settings->value("fitsImage").toBool()) {
        /* Add acquire time, date, and timestamp to the header */
        pFits->pHDU().addKey("EXPOSURE", exp_time,"Total Exposure Time (seconds)"); 
        pFits->pHDU().addKey("DATE-OBS", fits_date,"Date of Observation"); 
        pFits->pHDU().addKey("TSTAMP", (long)(timestamp.toMSecsSinceEpoch()/1000), 
                             "Start of Observation (seconds since epoch)"); 
        pFits->pHDU().addKey("EXPSTART", 
                             timestamp.toString("yyyy-MM-dd - hh:mm:ss.zzz").toUtf8().constData(), 
                             "Beginning of Observation in UTC"); 

        
        try {
            pFits->pHDU().write(fpixel, nelements, pixelArray);
        } catch(const FitsError &fe) {
            /* Tell us what happend if this borks */
            std::cerr << fe.message() << std::endl;
            return -1;
        }
    }

    qDebug() << "Writing binary table";

    /* Create photon list binary table */
    Table* plTable = pFits->addTable(plhduname, tableRows, colName, colForm, colUnit);

    /* Add useful header info to extension as well */
    pFits->extension(1).addKey("EXPOSURE", exp_time,"Total Exposure Time (seconds)"); 
    pFits->extension(1).addKey("DATE-OBS", fits_date,"Date of Observation"); 
    pFits->extension(1).addKey("TSTAMP", (long)(timestamp.toMSecsSinceEpoch()/1000), 
                               "Start of Observation (seconds since epoch)"); 
    pFits->extension(1).addKey("EXPSTART", 
                               timestamp.toString("yyyy-MM-dd - hh:mm:ss.zzz").toUtf8().constData(), 
                               "Beginning of Observation in UTC"); 
    
    /* Write x and y to table */
    plTable->column(colName[0]).write(xArray, 1);
    plTable->column(colName[1]).write(yArray, 1);
    plTable->column(colName[2]).write(pArray, 1);
    plTable->column(colName[3]).write(tArray, 1);

    qDebug() << "FITS compelte";

    /* uncomment to see hdu stuff */
    //std::cout << pFits->pHDU() << std::endl;

    return 0;
}
