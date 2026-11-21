//  actions used by the portals
void list_doctors() {
    cout << "\nid\tname\t\tspecialty\t\tstatus\t\tslots\tqueue\tfee\n";
    for (int i = 0; i < doctor_count; i++) {
        cout << doctors[i].id << "\t" << doctors[i].name << "\t" << doctors[i].specialty << "\t"
             << doctors[i].status << "\t" << doctors[i].slots << "\t"
             << waiting_list.length_for(doctors[i].id) << "\t" << doctors[i].fee << "\n";
    }
}

void patient_details(string id) {
    int p = find_patient(id);
    if (p == -1) { cout << "patient not found\n"; return; }
    cout << "\nid: " << patients[p].id << "\nname: " << patients[p].name
         << "\nage: " << patients[p].age << "\n";
    if (!patients[p].has_intake) { cout << "latest intake: not recorded yet\n"; return; }
    for (int j = 1; j < vital_count; j++) cout << vital_names[j] << ": " << patients[p].vitals[j] << "\n";
    cout << "symptoms:";
    for (int s = 0; s < symptom_count; s++) if (patients[p].symptoms[s]) cout << " " << symptom_names[s];
    cout << "\n";
}

void patient_reports(string id) {
    int p = find_patient(id);
    if (p == -1) { cout << "patient not found\n"; return; }
    cout << "\nmedical history of " << patients[p].name << "\n";
    if (patients[p].history == nullptr) { cout << "no visits yet\n"; return; }
    int number = 1;
    for (visit* v = patients[p].history; v != nullptr; v = v->next)
        cout << number++ << ". " << v->date << " | " << v->category << " | medicine: "
             << v->medicine << " | doctor: " << v->doctor_id << "\n";
}

// throws a message when the booking is not possible
void add_appointment(string patient_id, string doctor_id, int urgency, string category) {
    int d = find_doctor(doctor_id);
    if (d == -1) throw "doctor id not found";
    if (doctors[d].status == "Unavailable") throw "this doctor is unavailable today";
    if (waiting_list.length_for(doctor_id) >= doctors[d].slots) throw "no slots left for this doctor today";
    if (waiting_list.position_of(patient_id, doctor_id) > 0) throw "patient already has an appointment with this doctor";
    if (waiting_list.total >= max_appointments) throw "the queue is full";
    appointment a;
    a.patient_id = patient_id;
    a.doctor_id = doctor_id;
    a.urgency = urgency;
    a.category = category;
    waiting_list.add(a);
    save_appointments();
}

int ask_urgency() {
    int choice;
    cout << "urgency: 1 urgent, 2 priority, 3 routine\nchoose: ";
    cin >> choice;
    if (choice < 1 || choice > 3) {
        cout << "wrong choice, routine selected\n";
        choice = 3;
    }
    return choice - 1;
}

void show_position(string patient_id, string doctor_id) {
    int d = find_doctor(doctor_id);
    int place = waiting_list.position_of(patient_id, doctor_id);
    cout << "booked with " << doctors[d].name << ". queue position " << place
         << ", estimated wait " << (place - 1) * minutes_per_patient << " minutes\n";
}

// patient books a routine appointment, receptionist can also choose the urgency
void book_appointment(string patient_id, bool by_receptionist) {
    string doctor_id;
    list_doctors();
    cout << "doctor id: ";
    cin >> doctor_id;
    int urgency = 2;
    if (by_receptionist) urgency = ask_urgency();
    try {
        add_appointment(patient_id, doctor_id, urgency, "-");
        show_position(patient_id, doctor_id);
    } catch (const char* message) {
        cout << message << "\n";
    }
}

void display_appointments(string patient_id) {
    cout << "\nmy appointments\n";
    bool found = false;
    for (int i = 0; i < waiting_list.total; i++) {
        appointment& a = waiting_list.items[i];
        if (a.patient_id != patient_id) continue;
        found = true;
        cout << "with " << doctors[find_doctor(a.doctor_id)].name << " (" << a.doctor_id << ") - "
             << urgency_names[a.urgency] << "\n";
    }
    if (!found) cout << "no appointments\n";
}

void track_queue_status(string patient_id) {
    cout << "\nqueue status\n";
    bool found = false;
    for (int d = 0; d < doctor_count; d++) {
        int place = waiting_list.position_of(patient_id, doctors[d].id);
        if (place == 0) continue;
        found = true;
        cout << doctors[d].name << ": position " << place << " of " << waiting_list.length_for(doctors[d].id)
             << ", estimated wait " << (place - 1) * minutes_per_patient << " minutes\n";
    }
    if (!found) cout << "you are not in any queue\n";
}

// a doctor's line, in the order the patients will be called
void view_appointment(string doctor_id) {
    int d = find_doctor(doctor_id);
    if (d == -1) { cout << "doctor id not found\n"; return; }
    cout << "\nqueue of " << doctors[d].name << "\n";
    int place = 0;
    for (int i = 0; i < waiting_list.total; i++) {
        appointment& a = waiting_list.items[i];
        if (a.doctor_id != doctor_id) continue;
        place++;
        int p = find_patient(a.patient_id);
        cout << place << ". " << a.patient_id << " " << patients[p].name << " - "
             << urgency_names[a.urgency] << " - suspected: " << a.category << "\n";
    }
    if (place == 0) cout << "no appointments\n";
}

// adds a new patient and saves him or her in patients.csv
void register_new_patient() {
    if (patient_count == max_patients) { cout << "patient list is full\n"; return; }
    patient& p = patients[patient_count];
    cout << "patient name: ";
    cin.ignore();
    getline(cin, p.name);
    cout << "age: ";
    cin >> p.age;
    cout << "set a pin: ";
    cin >> p.pin;
    string number = to_string(patient_count + 1);
    while (number.size() < 3) number = "0" + number;
    p.id = "P" + number;
    p.vitals[0] = p.age;
    patient_count++;
    save_patients();
    cout << "patient saved in patients.csv, patient id is " << p.id << "\n";
}

// doctor (or receptionist for the doctor) saves the confirmed diagnosis and medicine
void report_upload(string doctor_id) {
    string patient_id, medicine;
    int pick;
    cout << "patient id: ";
    cin >> patient_id;
    int p = find_patient(patient_id);
    if (p == -1) { cout << "patient not registered\n"; return; }
    if (doctor_id == "") {
        cout << "doctor id: ";
        cin >> doctor_id;
        if (find_doctor(doctor_id) == -1) { cout << "doctor id not found\n"; return; }
    }

    cout << "confirmed diagnosis:\n";
    for (int i = 0; i < knn.category_count; i++)
        cout << i + 1 << ". " << knn.categories[i] << " (" << knn.specialists[i] << ")\n";
    cout << "choose number: ";
    cin >> pick;
    if (pick < 1 || pick > knn.category_count) { cout << "wrong choice\n"; return; }
    int urgency = ask_urgency();
    cout << "prescribed medicine: ";
    cin.ignore();
    getline(cin, medicine);

    patients[p].add_visit(today(), knn.categories[pick - 1], medicine, doctor_id);
    save_history();

    // the confirmed case becomes a new row in Book1.csv, so the next predictions get better
    if (patients[p].has_intake && knn.case_count < max_cases) {
        medical_case c;
        string number = to_string(knn.next_case_number++);
        while (number.size() < 3) number = "0" + number;
        c.id = "CASE" + number;
        for (int j = 0; j < vital_count; j++) c.vitals[j] = patients[p].vitals[j];
        for (int s = 0; s < symptom_count; s++) c.symptoms[s] = patients[p].symptoms[s];
        c.category = pick - 1;
        c.urgency = urgency;
        knn.add_case(c);
        refresh_knowledge();
        add_case_to_file(c);
        cout << c.id << " added to Book1.csv and to the k-nn data\n";
    } else {
        cout << "no intake data for this patient, so the case was not added to the k-nn data\n";
    }

    waiting_list.remove(patient_id, doctor_id);       // consultation finished
    save_appointments();
    cout << "report saved\n";
}

// ============================================================
