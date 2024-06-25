//============================================================================
// Name        : ProjectTwo.cpp
// Author      : Jewelia England
// Version     : 1.0
// Copyright   : Copyright © 2024 SNHU COCE
// Description : Project 2
//============================================================================

#include <algorithm>
#include <iostream>
#include <time.h>
#include <Windows.h>
#include <stdexcept>
#include <string>
#include <vector>
#include <list>
#include <sstream>
#include <fstream>
#include <iomanip>

using namespace std;

class Error : public std::runtime_error
{

public:
    Error(const std::string& msg) :
        std::runtime_error(std::string("CSVparser : ").append(msg))
    {
    }
};

class Row
{
public:
    Row(const std::vector<std::string>&);
    ~Row(void);

public:
    unsigned int size(void) const;
    void push(const std::string&);
    bool set(const std::string&, const std::string&);

private:
    const std::vector<std::string> _header;
    std::vector<std::string> _values;

public:

    template<typename T>
    const T getValue(unsigned int pos) const
    {
        if (pos < _values.size())
        {
            T res;
            std::stringstream ss;
            ss << _values[pos];
            ss >> res;
            return res;
        }
        throw Error("can't return this value (doesn't exist)");
    }
    const std::string operator[](unsigned int) const;
    const std::string operator[](const std::string& valueName) const;
    friend std::ostream& operator<<(std::ostream& os, const Row& row);
    friend std::ofstream& operator<<(std::ofstream& os, const Row& row);
};

enum DataType {
    eFILE = 0,
    ePURE = 1
};

class Parser
{

public:
    Parser(const std::string&, const DataType& type = eFILE, char sep = ',');
    ~Parser(void);

public:
    Row& getRow(unsigned int row) const;
    unsigned int rowCount(void) const;
    unsigned int columnCount(void) const;
    std::vector<std::string> getHeader(void) const;
    const std::string getHeaderElement(unsigned int pos) const;
    const std::string& getFileName(void) const;

public:
    bool deleteRow(unsigned int row);
    bool addRow(unsigned int pos, const std::vector<std::string>&);
    void sync(void) const;

protected:
    void parseHeader(void);
    void parseContent(void);

private:
    std::string _file;
    const DataType _type;
    const char _sep;
    std::vector<std::string> _originalFile;
    std::vector<std::string> _header;
    std::vector<Row*> _content;

public:
    Row& operator[](unsigned int row) const;
};
Parser::Parser(const std::string& data, const DataType& type, char sep)
    : _type(type), _sep(sep)
{
    std::string line;
    if (type == eFILE)
    {
        _file = data;
        std::ifstream ifile(_file.c_str());
        if (ifile.is_open())
        {
            while (ifile.good())
            {
                getline(ifile, line);
                if (line != "")
                    _originalFile.push_back(line);
            }
            ifile.close();

            if (_originalFile.size() == 0)
                throw Error(std::string("No Data in ").append(_file));

            parseHeader();
            parseContent();
        }
        else
            throw Error(std::string("Failed to open ").append(_file));
    }
    else
    {
        std::istringstream stream(data);
        while (std::getline(stream, line))
            if (line != "")
                _originalFile.push_back(line);
        if (_originalFile.size() == 0)
            throw Error(std::string("No Data in pure content"));

        parseHeader();
        parseContent();
    }
}

Parser::~Parser(void)
{
    std::vector<Row*>::iterator it;

    for (it = _content.begin(); it != _content.end(); it++)
        delete* it;
}

void Parser::parseHeader(void)
{
    std::stringstream ss(_originalFile[0]);
    std::string item;

    while (std::getline(ss, item, _sep))
        _header.push_back(item);
}

void Parser::parseContent(void)
{
    std::vector<std::string>::iterator it;

    it = _originalFile.begin();
    it++; // skip header

    for (; it != _originalFile.end(); it++)
    {
        bool quoted = false;
        int tokenStart = 0;
        unsigned int i = 0;

        Row* row = new Row(_header);

        for (; i != it->length(); i++)
        {
            if (it->at(i) == '"')
                quoted = ((quoted) ? (false) : (true));
            else if (it->at(i) == ',' && !quoted)
            {
                row->push(it->substr(tokenStart, i - tokenStart));
                tokenStart = i + 1;
            }
        }

        //end
        row->push(it->substr(tokenStart, it->length() - tokenStart));

        // if value(s) missing
        if (row->size() != _header.size())
            throw Error("corrupted data !");
        _content.push_back(row);
    }
}

Row& Parser::getRow(unsigned int rowPosition) const
{
    if (rowPosition < _content.size())
        return *(_content[rowPosition]);
    throw Error("can't return this row (doesn't exist)");
}

Row& Parser::operator[](unsigned int rowPosition) const
{
    return Parser::getRow(rowPosition);
}

unsigned int Parser::rowCount(void) const
{
    return _content.size();
}

unsigned int Parser::columnCount(void) const
{
    return _header.size();
}

std::vector<std::string> Parser::getHeader(void) const
{
    return _header;
}

const std::string Parser::getHeaderElement(unsigned int pos) const
{
    if (pos >= _header.size())
        throw Error("can't return this header (doesn't exist)");
    return _header[pos];
}

bool Parser::deleteRow(unsigned int pos)
{
    if (pos < _content.size())
    {
        delete* (_content.begin() + pos);
        _content.erase(_content.begin() + pos);
        return true;
    }
    return false;
}

bool Parser::addRow(unsigned int pos, const std::vector<std::string>& r)
{
    Row* row = new Row(_header);

    for (auto it = r.begin(); it != r.end(); it++)
        row->push(*it);

    if (pos <= _content.size())
    {
        _content.insert(_content.begin() + pos, row);
        return true;
    }
    return false;
}

void Parser::sync(void) const
{
    if (_type == DataType::eFILE)
    {
        std::ofstream f;
        f.open(_file, std::ios::out | std::ios::trunc);

        // header
        unsigned int i = 0;
        for (auto it = _header.begin(); it != _header.end(); it++)
        {
            f << *it;
            if (i < _header.size() - 1)
                f << ",";
            else
                f << std::endl;
            i++;
        }

        for (auto it = _content.begin(); it != _content.end(); it++)
            f << **it << std::endl;
        f.close();
    }
}

const std::string& Parser::getFileName(void) const
{
    return _file;
}

/*
** ROW
*/

Row::Row(const std::vector<std::string>& header)
    : _header(header) {}

Row::~Row(void) {}

unsigned int Row::size(void) const
{
    return _values.size();
}

void Row::push(const std::string& value)
{
    _values.push_back(value);
}

bool Row::set(const std::string& key, const std::string& value)
{
    std::vector<std::string>::const_iterator it;
    int pos = 0;

    for (it = _header.begin(); it != _header.end(); it++)
    {
        if (key == *it)
        {
            _values[pos] = value;
            return true;
        }
        pos++;
    }
    return false;
}

const std::string Row::operator[](unsigned int valuePosition) const
{
    if (valuePosition < _values.size())
        return _values[valuePosition];
    throw Error("can't return this value (doesn't exist)");
}

const std::string Row::operator[](const std::string& key) const
{
    std::vector<std::string>::const_iterator it;
    int pos = 0;

    for (it = _header.begin(); it != _header.end(); it++)
    {
        if (key == *it)
            return _values[pos];
        pos++;
    }

    throw Error("can't return this value (doesn't exist)");
}

std::ostream& operator<<(std::ostream& os, const Row& row)
{
    for (unsigned int i = 0; i != row._values.size(); i++)
        os << row._values[i] << " | ";

    return os;
}

std::ofstream& operator<<(std::ofstream& os, const Row& row)
{
    for (unsigned int i = 0; i != row._values.size(); i++)
    {
        os << row._values[i];
        if (i < row._values.size() - 1)
            os << ",";
    }
    return os;
}


//============================================================================
// Global definitions visible to all methods and classes
//============================================================================

// forward declarations
double strToDouble(string str, char ch);

// define a structure to hold course information
struct Course {
    string courseId; // unique identifier
    string title;
    string prerequisites;
    double amount;
    Course() {
        amount = 0.0;
    }
};
vector<Course> courses;
//============================================================================
// Static methods used for testing
//============================================================================

/**
 * Display the course information to the console (std::out)
 *
 * @param course struct containing the course info
 */
void displayCourse(Course course) {
    std::cout << course.title << ": " << course.courseId << " | " << course.amount << " | "
        << course.prerequisites << endl;
    return;
}
/**
* Search for the specified bidId
*
* @param bidId The bid id to search for
*/
Course SearchCourse(string courseId) {
    Course course;

    // FIXME (8): Implement logic to search for and return a bid

        // FIXME (6): Implement logic to print all bids

    unsigned int idx = 0;

    //Iterate the loop
    for (int i = 0; i < courses.size(); ++i) {
        if (courses[i].courseId._Equal(courseId))
        {
            return courses[i];
        }
    }
    return course;
}
/**
 * Load a CSV file containing courses into a container
 *
 * @param csvPath the path to the CSV file to load
 * @return a container holding all the courses read
 */
vector<Course> loadCourses(string csvPath) {
    std::cout << "Loading CSV file " << csvPath << endl;

    // Define a vector data structure to hold a collection of courses.
    vector<Course> courses;

    // initialize the CSV Parser using the given path
    Parser file = Parser(csvPath);

    try {
        // loop to read rows of a CSV file
        for (int i = 0; i < file.rowCount(); i++) {

            // Create a data structure and add to the collection of courses
            Course course;
            // JOE course.courseId = file[i][1];
            // JOE course.title = file[i][0];

            course.title = file[i][1];
            course.courseId = file[i][0];


            course.prerequisites = file[i][2];
            //course.amount = strToDouble(file[i][4], '$');

            cout << "Item: " << course.title << ", prerequisites: " << course.prerequisites << ", Amount: " << course.amount << endl;

            // push this course to the end
            courses.push_back(course);
        }
    }
    catch (Error& e) {
        std::cerr << e.what() << std::endl;
    }
    return courses;
}

// FIXME (2a): Implement the quick sort logic over course.title

/**
 * Partition the vector of courses into two parts, low and high
 *
 * @param courses Address of the vector<course> instance to be partitioned
 * @param begin Beginning index to partition
 * @param end Ending index to partition
 */
int partition(vector<Course>& courses, int begin, int end) {
    //set low and high equal to begin and end

    // pick the middle element as pivot point

    // while not done 

        // keep incrementing low index while courses[low] < courses[pivot]

        // keep decrementing high index while courses[pivot] < courses[high]

        /* If there are zero or one elements remaining,
            all courses are partitioned. Return high */
            // else swap the low and high courses (built in vector method)
                 // move low and high closer ++low, --high
         //return high;


         //local variable declaration
    int midPoint;
    string pivot;
    Course tempSwap;
    bool done;

    midPoint = begin + (end - begin) / 2;
    pivot = courses[midPoint].title;
    done = false;

    while (!done) {

        while (courses[begin].title < pivot) {

            begin++;
        }

        while (pivot < courses[end].title) {

            end--;
        }

        if (begin >= end) {

            done = true;
        }

        else {

            //set-up the swaps using a tempSwap variable of type course
            tempSwap = courses[begin];
            courses[begin] = courses[end];
            courses[end] = tempSwap;

            begin++;
            end--;
        }
    }


    return end;
}

/**
 * Perform a quick sort on course title
 * Average performance: O(n log(n))
 * Worst case performance O(n^2))
 *
 * @param courses address of the vector<course> instance to be sorted
 * @param begin the beginning index to sort on
 * @param end the ending index to sort on
 */
void quickSort(vector<Course>& courses, int begin, int end) {
    //set mid equal to 0

    /* Base case: If there are 1 or zero courses to sort,
     partition is already sorted otherwise if begin is greater
     than or equal to end then return*/

     /* Partition courses into low and high such that
      midpoint is location of last element in low */

      // recursively sort low partition (begin to mid)

      // recursively sort high partition (mid+1 to end)

    int lowIndex;

    if (begin >= end) {

        return;
    }

    //call the partition function to define the lowIndex
    lowIndex = partition(courses, begin, end);

    //recursive calls to quickSort
    quickSort(courses, begin, lowIndex);
    quickSort(courses, lowIndex + 1, end);

}

// FIXME (1a): Implement the selection sort logic over course.title

/**
 * Perform a selection sort on course title
 * Average performance: O(n^2))
 * Worst case performance O(n^2))
 *
 * @param course address of the vector<course>
 *            instance to be sorted
 */
void selectionSort(vector<Course>& courses) {
    //define min as int (index of the current minimum course)

    // check size of courses vector
    // set size_t platform-neutral result equal to course.size()

    // pos is the position within courses that divides sorted/unsorted
    // for size_t pos = 0 and less than size -1 
        // set min = pos
        // loop over remaining elements to the right of position
            // if this element's title is less than minimum title
                // this element becomes the minimum
        // swap the current minimum with smaller one found
            // swap is a built in vector method

    //local variable declaration
    int indexSmallest = 0;
    Course tempSwap;

    for (int i = 0; i < courses.size() - 1; i++) {

        indexSmallest = i;

        for (int j = i + 1; j < courses.size(); j++) {

            if (courses[j].title < courses[indexSmallest].title) {
                indexSmallest = j;
            }
        }

        //swap the vector positions using a tempSwap variable of type course
        tempSwap = courses[i];
        courses[i] = courses[indexSmallest];
        courses[indexSmallest] = tempSwap;

    }
}

/**
 * Simple C function to convert a string to a double
 * after stripping out unwanted char
 *
 * credit: http://stackoverflow.com/a/24875936
 *
 * @param ch The character to strip out
 */
double strToDouble(string str, char ch) {
    str.erase(remove(str.begin(), str.end(), ch), str.end());
    return atof(str.c_str());
}

/**
 * The one and only main() method
 */
int main(int argc, char* argv[]) {

    // process command line arguments
    string csvPath;
    switch (argc) {
    case 2:
        csvPath = argv[1];
        break;
    default:
        csvPath = "C : \repos\ProjectTwo\data\test.txt";
    }

    // Define a vector to hold all the courses
    //JOE vector<Course> courses;

    // Define a timer variable
    clock_t ticks;

    //add a few local variables for try/catch and usability needs

    const int GLOBAL_SLEEP_VALUE = 5000;
    int choice = 0;
    string anyKey = " ";
    bool goodInput;
    Course course1;
    string courseSearch;

    while (choice != 9) {

        std::cout << "Menu:" << endl;
        std::cout << "  1. Load courses" << endl;
        std::cout << "  2. Display All courses" << endl;
        std::cout << "  3. Selection Sort All courses" << endl;
        std::cout << "  4. Quick Sort All courses" << endl;
        std::cout << "  5. Find Course" << endl;
        std::cout << "  9. Exit" << endl;
        std::cout << "Enter choice: ";

        try { //add a try catch to protect against bad input

            std::cin >> choice;

            if ((choice > 0 && choice < 6) || (choice == 9)) {// limit the user menu inputs to good values
                goodInput = true;
            }
            else {//throw error for catch
                goodInput = false;
                throw 1;
            }

            switch (choice) { //create a swtich to allow the menu to work

            case 1:
                // Initialize a timer variable before loading courses
                ticks = clock();

                // Complete the method call to load the courses
                courses = loadCourses(csvPath);

                std::cout << courses.size() << " courses read" << endl;

                // Calculate elapsed time and display result
                ticks = clock() - ticks; // current clock ticks minus starting clock ticks
                std::cout << "time: " << ticks << " clock ticks" << endl;
                std::cout << "time: " << ticks * 1.0 / CLOCKS_PER_SEC << " seconds" << endl;

                Sleep(GLOBAL_SLEEP_VALUE);

                break;

            case 2:
                // Loop and display the courses read
                for (int i = 0; i < courses.size(); ++i) {
                    displayCourse(courses[i]);
                }
                std::cout << "Press any key to continue...";

                std::cin >> anyKey;
                Sleep(GLOBAL_SLEEP_VALUE);

                break;

            case 3:

                //selection sort switch 
                //start the clock with the tick variable and then call the function
                //stop the clock with tick again and then outpout the time it tookto run
                //sleep for some amount of time and then redraw the menu

                ticks = clock();

                selectionSort(courses);

                // Calculate elapsed time and display result
                ticks = clock() - ticks; // current clock ticks minus starting clock ticks
                std::cout << "time: " << ticks << " clock ticks" << endl;
                std::cout << "time: " << ticks * 1.0 / CLOCKS_PER_SEC << " seconds" << endl;

                Sleep(GLOBAL_SLEEP_VALUE);

                break;

            case 4:

                //quick sort switch 
                //start the clock with the tick variable and then call the function
                //stop the clock with tick again and then outpout the time it took to run
                //sleep for some amount of time and then redraw the menu

                ticks = clock();

                quickSort(courses, 0, courses.size() - 1);

                // Calculate elapsed time and display result
                ticks = clock() - ticks; // current clock ticks minus starting clock ticks
                std::cout << "time: " << ticks << " clock ticks" << endl;
                std::cout << "time: " << ticks * 1.0 / CLOCKS_PER_SEC << " seconds" << endl;

                Sleep(GLOBAL_SLEEP_VALUE);

                break;

            case 5:
                 
                std::cout << "Enter cource to search for:" << endl;
                std::cin >> courseSearch;
                ticks = clock();
                //course1 = SearchCourse("CSCI100");
                course1 = SearchCourse(courseSearch);
                if (!course1.courseId._Equal(""))
                {
                    std::cout << course1.courseId << ": " << course1.title << " | " << course1.amount << " | "
                        << course1.prerequisites << endl;
                }
                else
                {
                    std::cout << "Could not find course " << course1.title << endl;
                }

                std::cout << "Press any key to continue...";

                std::cin >> anyKey;

                /*JOE
                bid = bidTable->Search(bidKey);

                ticks = clock() - ticks; // current clock ticks minus starting clock ticks

                if (!bid.bidId.empty()) {
                    displayBid(bid);
                }
                else {
                    cout << "Bid Id " << bidKey << " not found." << endl;
                }

                cout << "time: " << ticks << " clock ticks" << endl;
                cout << "time: " << ticks * 1.0 / CLOCKS_PER_SEC << " seconds" << endl;
                */
                break;

            case 9:
                //default case for the exit statement so we don't fail the try catch

                break;

            default:
                throw 2;
            }
        }
        catch (int err) {
            std::cout << "\nPlease check your input." << endl;
            Sleep(GLOBAL_SLEEP_VALUE);
        }

        //need to clear the cin operator of extra input, e.g., 9 9, or any errors generated by bad input, e.g., 'a'
        cin.clear();
        cin.ignore();

        //clear the consolse to redraw a fresh menu
        system("cls");
    }

    std::cout << "Good bye." << endl;

    Sleep(GLOBAL_SLEEP_VALUE);

    return 0;
}

