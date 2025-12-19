# ScholarSphere: Student Record Management System
ScholarSphere is a robust, console-based application developed in C++ to streamline the management of student records. It replaces manual data handling with an automated system that supports comprehensive CRUD operations, role-based access control, and persistent data storage.

![Title Page](Scholarsphere\Final Project - Comp Prog\images\titlepage.jpg)

## ✨ Key Features

### 🔐 Role-Based Authentication
* **Administrator Access**: Full system control using a secure login (Password: admin123). Admins can register students, modify any record, and oversee the entire database.

* **Student Access**: Restricted access where students can view their personal profiles, check grades, and manage their financial balance using their Student ID.

### 📊 Academic & Grade Management
* **Automatic GPA Calculation**: Enter subject grades (0-100), and the system automatically calculates the average and assigns a letter grade (A-F).

* **Course Tracking**: Supports different courses (BSCS, BSIS, BSIT) with specific section assignments.

### 💳 Financial & Scholarship Module

* **Wallet System**: Students can deposit funds into a virtual wallet to pay tuition.

* **Tuition Management**: Calculates tuition based on the number of enrolled subjects and tracks outstanding balances.

* **Scholarship Status**: Indicates whether a student is currently a scholar for the semester.

### 💾 Data Persistence & Security

* **File I/O System**: All student records are saved to students_data.txt, ensuring information is preserved even after the program is closed.

* **Confirmation Prompts**: Includes safety checks before deleting records to prevent accidental data loss.


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
