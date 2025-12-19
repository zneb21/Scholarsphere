# ScholarSphere: Student Record Management System
ScholarSphere is a robust, console-based application developed in C++ to streamline the management of student records. It replaces manual data handling with an automated system that supports comprehensive CRUD operations, role-based access control, and persistent data storage.

## 🚀 Features

### 1. User Authentication & Access Control

The system separates permissions between two types of users:

* **Admin**: Requires a password (`admin123`) to access full management tools, including adding, updating, or deleting any student record.
* 
**Student**: Restricted to viewing their own profile or deleting their own account after verifying their Student ID and Name.



### 2. Core Management (CRUD)

* 
**Create**: Register new students with detailed information (ID, Name, Age, Course, Section, Contact, E-mail, Birthday, and Blood Type).


* 
**Read**: Search for students by name or course, or view a complete list automatically sorted alphabetically.


* 
**Update**: Modify existing personal details or manage a student's subjects and grades.


* 
**Delete**: Permanently remove records with a mandatory confirmation step to prevent accidental data loss.



### 3. Academic & Financial Utilities

* **Grade Calculation**: Automatically calculates a student's average and assigns letter grades (A-F) based on subject performance.
* **Financial Tracking**: Tracks tuition fees (based on number of subjects), payments made, and current wallet balances.
* **Scholarship Status**: Displays whether a student has an active scholarship for the current semester.

### 4. System Robustness

* 
**Data Persistence**: Automatically saves all changes to `students_data.txt` and reloads them upon startup, ensuring no data is lost between sessions.


* 
**Input Validation**: Restricts grades to values between 0-100 and prevents system crashes from incorrect data types.


* 
**Live Context**: Features a real-time date and time display on the interface.



## 🛠️ Technical Setup for VS Code

### Prerequisites

* **C++ Compiler**: GCC/MinGW (Windows) or Clang (Mac/Linux).
* **VS Code Extensions**: Install the **C/C++** extension by Microsoft.

### Installation & Execution

1. **Clone the Project**: Ensure `ScholarSphere.cpp` is in your workspace folder.
2. **Compilation**: Open the terminal in VS Code (`Ctrl+``) and run:
```bash
g++ ScholarSphere.cpp -o ScholarSphere

```


3. **Run**:
* **Windows**: `.\ScholarSphere.exe`
* **Mac/Linux**: `./ScholarSphere`



## 📂 File Structure

* `ScholarSphere.cpp`: The main source code containing system logic and UI.
* `students_data.txt`: The database file where student records are persistently stored.

## 👥 Authors

This project was developed by **BSCS 1-A** students at **West Visayas State University**:

* Nico Andrei Campania 

* Zneb John D. Delariman
