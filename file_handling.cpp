//  reading and writing files
// 20 caseCategory, 21 urgencyLabel, 22 recommendedSpecialist
void read_cases() {
    ifstream file(cases_file);
    if (!file) throw "cannot open Book1.csv - keep it in the same folder as the program";
    string line, parts[30];
    getline(file, line);                       // first line is the heading
    while (getline(file, line)) {
        int found = split_line(line, parts, 30);
        if (found < 23 || parts[0] == "" || knn.case_count == max_cases) continue;   // skips the empty excel rows
        medical_case c;
        c.id = parts[0];
        for (int i = 0; i < vital_count; i++) c.vitals[i] = atof(parts[2 + i].c_str());
        for (int i = 0; i < symptom_count; i++) c.symptoms[i] = atoi(parts[9 + i].c_str());
        c.category = knn.find_category(parts[20]);
        if (c.category == -1) c.category = knn.add_category(parts[20], parts[22]);
        c.urgency = 2;
        for (int u = 0; u < 3; u++) if (parts[21] == urgency_labels[u]) c.urgency = u;
        if (c.id.size() > 4 && atoi(c.id.c_str() + 4) >= knn.next_case_number)
            knn.next_case_number = atoi(c.id.c_str() + 4) + 1;
        knn.add_case(c);
    }
    if (knn.case_count == 0) throw "no cases found in Book1.csv";
    refresh_knowledge();
    cout << "loaded " << knn.case_count << " cases from " << cases_file << "\n";
}

// columns of doctor_details.csv: 0 id, 1 name, 2 department, 3 specialty,
// 4 status, 5 experience, 6 fee, 7 slots today
void read_doctors() {
    ifstream file(doctors_file);
    if (!file) throw "cannot open doctor_details.csv - keep it in the same folder as the program";
    string line, parts[30];
    getline(file, line);
    while (getline(file, line)) {
        int found = split_line(line, parts, 30);
        if (found < 8 || parts[0] == "" || doctor_count == max_doctors) continue;
        doctors[doctor_count].id = parts[0];
        doctors[doctor_count].name = parts[1];
        doctors[doctor_count].specialty = parts[3];
        doctors[doctor_count].status = parts[4];
        doctors[doctor_count].fee = atoi(parts[6].c_str());
        doctors[doctor_count].slots = atoi(parts[7].c_str());
        doctors[doctor_count].pin = "1234";            // the file has no pin column
        doctor_count++;
    }
    if (doctor_count == 0) throw "no doctors found in doctor_details.csv";
    cout << "loaded " << doctor_count << " doctors from " << doctors_file << "\n";
}

void save_patients() {
    ofstream file("patients.csv");
    file << "patient_id,name,pin,age,has_intake,temperature,heart_rate,systolic,diastolic,oxygen,glucose,symptom_bits\n";
    for (int i = 0; i < patient_count; i++) {
        patient& p = patients[i];
        file << p.id << "," << p.name << "," << p.pin << "," << p.age << "," << p.has_intake;
        for (int j = 1; j < vital_count; j++) file << "," << p.vitals[j];
        file << ",";
        for (int s = 0; s < symptom_count; s++) file << p.symptoms[s];
        file << "\n";
    }
}

void save_history() {
    ofstream file("history.csv");
    file << "patient_id,date,category,medicine,doctor_id\n";
    for (int i = 0; i < patient_count; i++) {
        for (visit* v = patients[i].history; v != nullptr; v = v->next)
            file << patients[i].id << "," << v->date << "," << v->category << ","
                 << v->medicine << "," << v->doctor_id << "\n";
    }
}

void save_appointments() {
    ofstream file("appointments.csv");
    file << "patient_id,doctor_id,urgency,seq,category\n";
    for (int i = 0; i < waiting_list.total; i++) {
        appointment& a = waiting_list.items[i];
        file << a.patient_id << "," << a.doctor_id << "," << a.urgency << ","
             << a.seq << "," << a.category << "\n";
    }
}

// adds one new row at the end of Book1.csv, same column order as the other rows
void add_case_to_file(medical_case& c) {
    ofstream file(cases_file, ios::app);
    file << c.id << ",NO";                              // synthetic = NO, it is a real confirmed case
    for (int j = 0; j < vital_count; j++) file << "," << c.vitals[j];
    for (int s = 0; s < symptom_count; s++) file << "," << c.symptoms[s];
    file << "," << knn.categories[c.category] << "," << urgency_labels[c.urgency]
         << "," << knn.specialists[c.category] << ",\r\n";
}

// loads patients, history and appointments saved by the last run (if the files exist)
void load_saved_data() {
    string line, parts[30];

    ifstream patient_file("patients.csv");
    if (patient_file) {
        getline(patient_file, line);
        while (getline(patient_file, line) && patient_count < max_patients) {
            if (split_line(line, parts, 30) < 12) continue;
            patient& p = patients[patient_count];
            p.id = parts[0];
            p.name = parts[1];
            p.pin = parts[2];
            p.age = atoi(parts[3].c_str());
            p.has_intake = (parts[4] == "1");
            p.vitals[0] = p.age;
            for (int j = 1; j < vital_count; j++) p.vitals[j] = atof(parts[4 + j].c_str());
            for (int s = 0; s < symptom_count && s < (int)parts[11].size(); s++) p.symptoms[s] = (parts[11][s] == '1');
            patient_count++;
        }
    }

    ifstream history_file("history.csv");
    if (history_file) {
        getline(history_file, line);
        while (getline(history_file, line)) {
            if (split_line(line, parts, 30) < 5) continue;
            int p = find_patient(parts[0]);
            if (p != -1) patients[p].add_visit(parts[1], parts[2], parts[3], parts[4]);
        }
    }

    ifstream appointment_file("appointments.csv");
    if (appointment_file) {
        getline(appointment_file, line);
        while (getline(appointment_file, line) && waiting_list.total < max_appointments) {
            if (split_line(line, parts, 30) < 5) continue;
            appointment a;
            a.patient_id = parts[0];
            a.doctor_id = parts[1];
            a.urgency = atoi(parts[2].c_str());
            a.seq = atoi(parts[3].c_str());
            a.category = parts[4];
            waiting_list.items[waiting_list.total++] = a;      // file is already in queue order
            if (a.seq >= waiting_list.next_seq) waiting_list.next_seq = a.seq + 1;
        }
    }
    if (patient_count > 0) cout << "loaded " << patient_count << " saved patients\n";
}

// ============================================================
