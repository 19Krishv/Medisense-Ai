//  users and portals
// ============================================================
void doctor_menu(string id);
void patient_menu(string id);
void receptionist_menu();

class user {
private:
    string user_id;
    string name;
public:
    void input(string id, string user_name) {
        user_id = id;
        name = user_name;
    }
    string get_id() { return user_id; }
    string get_name() { return name; }
    virtual void open_portal() = 0;           // every role opens its own portal
    virtual ~user() {}
};

class doctor_portal : public user {
public:
    void open_portal() {
        string id, pin;
        for (;;) {
            cout << "enter doctor id (0 to go back): ";
            cin >> id;
            if (id == "0") return;
            int d = find_doctor(id);
            if (d == -1) { cout << "doctor id not found\n"; continue; }
            cout << "enter pin: ";
            cin >> pin;
            if (pin == doctors[d].pin) {
                input(id, doctors[d].name);
                cout << "welcome, " << get_name() << "\n";
                doctor_menu(get_id());
                return;
            }
            cout << "wrong pin\n";
        }
    }
};

class patient_portal : public user {
public:
    void open_portal() {
        string id, pin;
        for (;;) {
            cout << "enter patient id (0 to go back): ";
            cin >> id;
            if (id == "0") return;
            int p = find_patient(id);
            if (p == -1) { cout << "patient id not found\n"; continue; }
            cout << "enter pin: ";
            cin >> pin;
            if (pin == patients[p].pin) {
                input(id, patients[p].name);
                cout << "welcome, " << get_name() << "\n";
                patient_menu(get_id());
                return;
            }
            cout << "wrong pin\n";
        }
    }
};

class receptionist_portal : public user {
public:
    void open_portal() {
        input("rec", "receptionist");
        receptionist_menu();
    }
};

void patient_menu(string id) {
    int choice;
    for (;;) {
        cout << "\npatient portal\n1 view details\n2 check reports\n3 book appointment\n"
                "4 view my appointments\n5 track queue status\n0 logout\nchoice: ";
        cin >> choice;
        switch (choice) {
            case 1: patient_details(id); break;
            case 2: patient_reports(id); break;
            case 3: book_appointment(id, false); break;
            case 4: display_appointments(id); break;
            case 5: track_queue_status(id); break;
            case 0: cout << "logged out\n"; return;
            default: cout << "invalid choice\n";
        }
    }
}

void receptionist_menu() {
    int choice;
    string id;
    for (;;) {
        cout << "\nreceptionist portal\n1 register patient\n2 book appointment for patient\n"
                "3 view a doctor's appointments\n4 list doctors\n5 run disease prediction\n"
                "6 record doctor's prescription\n0 logout\nchoice: ";
        cin >> choice;
        switch (choice) {
            case 1:
                register_new_patient();
                break;
            case 2:
                cout << "patient id: ";
                cin >> id;
                if (find_patient(id) != -1) book_appointment(id, true);
                else cout << "patient not registered\n";
                break;
            case 3:
                cout << "doctor id: ";
                cin >> id;
                view_appointment(id);
                break;
            case 4: list_doctors(); break;
            case 5: run_prediction(); break;
            case 6: report_upload(""); break;
            case 0: cout << "logged out\n"; return;
            default: cout << "invalid choice\n";
        }
    }
}

void doctor_menu(string id) {
    int choice;
    string patient_id;
    for (;;) {
        cout << "\ndoctor portal\n1 view my patients (queue)\n2 view patient details and history\n"
                "3 report upload (diagnosis and prescription)\n0 logout\nchoice: ";
        cin >> choice;
        switch (choice) {
            case 1:
                view_appointment(id);
                break;
            case 2:
                cout << "patient id: ";
                cin >> patient_id;
                if (find_patient(patient_id) != -1) {
                    patient_details(patient_id);
                    patient_reports(patient_id);
                } else {
                    cout << "patient not registered\n";
                }
                break;
            case 3: report_upload(id); break;
            case 0: cout << "logged out\n"; return;
            default: cout << "invalid choice\n";
        }
    }
}

// ============================================================
