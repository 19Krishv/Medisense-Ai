#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define CSV_FILE "Book1.csv"
#define MAX_CASES 500
#define MAX_LINE 2048
#define FEATURE_COUNT 5
#define SYMPTOM_COUNT 11
#define K_NEIGHBORS 5
#define MAX_CATEGORIES 50
#define TEXT_SIZE 64

/* Column indexes in the provided 23-column CSV (counting from zero). */
enum {
    COL_CASE_ID = 0,
    COL_HEART_RATE = 4,
    COL_SYSTOLIC_BP = 5,
    COL_DIASTOLIC_BP = 6,
    COL_OXYGEN = 7,
    COL_GLUCOSE = 8,
    COL_FEVER = 9,
    COL_CATEGORY = 20,
    COL_URGENCY = 21,
    COL_SPECIALTY = 22
};

static const char *symptom_names[SYMPTOM_COUNT] = {
    "Fever", "Cough", "Sore throat", "Chest pain", "Headache",
    "Vomiting", "Abdominal pain", "Fatigue", "Rash",
    "Frequent urination", "Blurred vision"
};

typedef struct {
    char case_id[TEXT_SIZE];
    double vital[FEATURE_COUNT];
    int symptom[SYMPTOM_COUNT];
    char category[TEXT_SIZE];
    char urgency[TEXT_SIZE];
    char specialty[TEXT_SIZE];
    double distance;
} CaseRecord;

typedef struct {
    char name[TEXT_SIZE];
    int knn_votes;
    double symptom_score;
    double combined_score;
} CategoryScore;

static void copy_text(char *destination, size_t capacity, const char *source)
{
    if (capacity == 0) return;
    snprintf(destination, capacity, "%s", source ? source : "");
}

static int parse_integer(const char *text)
{
    return text ? atoi(text) : 0;
}

static double parse_number(const char *text)
{
    return text ? strtod(text, NULL) : 0.0;
}

/* Load one CSV row. This project CSV has no commas inside quoted fields. */
static int load_cases(CaseRecord cases[], int capacity)
{
    FILE *file = fopen(CSV_FILE, "r");
    char line[MAX_LINE];
    int count = 0;

    if (file == NULL) {
        perror("Could not open patient CSV");
        return -1;
    }

    /* The first CSV line contains column names, so skip it. */
    if (fgets(line, sizeof(line), file) == NULL) {
        fclose(file);
        return 0;
    }

    while (count < capacity && fgets(line, sizeof(line), file) != NULL) {
        char *fields[23] = {0};
        int field_count = 0;
        char *token = strtok(line, ",\r\n");

        while (token != NULL && field_count < 23) {
            fields[field_count++] = token;
            token = strtok(NULL, ",\r\n");
        }

        /* Ignore malformed rows that do not contain all expected columns. */
        if (field_count < 23) continue;

        CaseRecord *record = &cases[count];
        copy_text(record->case_id, sizeof(record->case_id), fields[COL_CASE_ID]);

        /* k-NN features: heart rate, systolic BP, diastolic BP, oxygen, glucose. */
        record->vital[0] = parse_number(fields[COL_HEART_RATE]);
        record->vital[1] = parse_number(fields[COL_SYSTOLIC_BP]);
        record->vital[2] = parse_number(fields[COL_DIASTOLIC_BP]);
        record->vital[3] = parse_number(fields[COL_OXYGEN]);
        record->vital[4] = parse_number(fields[COL_GLUCOSE]);

        /* Symptom flags are columns 9 through 19 in the provided CSV. */
        for (int s = 0; s < SYMPTOM_COUNT; s++) {
            record->symptom[s] = parse_integer(fields[COL_FEVER + s]) != 0;
        }

        copy_text(record->category, sizeof(record->category), fields[COL_CATEGORY]);
        copy_text(record->urgency, sizeof(record->urgency), fields[COL_URGENCY]);
        copy_text(record->specialty, sizeof(record->specialty), fields[COL_SPECIALTY]);
        record->distance = 0.0;
        count++;
    }

    fclose(file);
    return count;
}

/* Scale each feature using the min/max values in the loaded historical cases. */
static void get_feature_ranges(const CaseRecord cases[], int count,
                               double minimum[], double maximum[])
{
    for (int f = 0; f < FEATURE_COUNT; f++) {
        minimum[f] = cases[0].vital[f];
        maximum[f] = cases[0].vital[f];

        for (int i = 1; i < count; i++) {
            if (cases[i].vital[f] < minimum[f]) minimum[f] = cases[i].vital[f];
            if (cases[i].vital[f] > maximum[f]) maximum[f] = cases[i].vital[f];
        }
    }
}

static double normalize(double value, double minimum, double maximum)
{
    if (maximum == minimum) return 0.0;
    return (value - minimum) / (maximum - minimum);
}

static int compare_distance(const void *left, const void *right)
{
    const CaseRecord *a = (const CaseRecord *)left;
    const CaseRecord *b = (const CaseRecord *)right;
    if (a->distance < b->distance) return -1;
    if (a->distance > b->distance) return 1;
    return 0;
}

static int find_category(CategoryScore categories[], int count, const char *name)
{
    for (int i = 0; i < count; i++) {
        if (strcmp(categories[i].name, name) == 0) return i;
    }
    return -1;
}

static int build_categories(const CaseRecord cases[], int case_count,
                            CategoryScore categories[])
{
    int category_count = 0;

    for (int i = 0; i < case_count; i++) {
        if (find_category(categories, category_count, cases[i].category) < 0 &&
            category_count < MAX_CATEGORIES) {
            copy_text(categories[category_count].name,
                      sizeof(categories[category_count].name), cases[i].category);
            categories[category_count].knn_votes = 0;
            categories[category_count].symptom_score = 0.0;
            categories[category_count].combined_score = 0.0;
            category_count++;
        }
    }
    return category_count;
}

/* Jaccard similarity: shared positive symptoms divided by all positive symptoms. */
static double symptom_similarity(const int input[], const int historical[])
{
    int intersection = 0;
    int union_count = 0;

    for (int i = 0; i < SYMPTOM_COUNT; i++) {
        if (input[i] || historical[i]) union_count++;
        if (input[i] && historical[i]) intersection++;
    }

    if (union_count == 0) return 0.0;
    return (double)intersection / union_count;
}

static void read_patient_input(double vital[], int symptom[])
{
    printf("Enter the patient's five demo feature values.\n");
    printf("Heart rate (bpm): ");
    scanf("%lf", &vital[0]);
    printf("Systolic blood pressure (mmHg): ");
    scanf("%lf", &vital[1]);
    printf("Diastolic blood pressure (mmHg): ");
    scanf("%lf", &vital[2]);
    printf("Oxygen level (%%): ");
    scanf("%lf", &vital[3]);
    printf("Glucose (mg/dL): ");
    scanf("%lf", &vital[4]);

    printf("\nEnter 1 for a symptom present, 0 for absent.\n");
    for (int i = 0; i < SYMPTOM_COUNT; i++) {
        printf("%s (0/1): ", symptom_names[i]);
        scanf("%d", &symptom[i]);
        symptom[i] = symptom[i] != 0;
    }
}

int main(void)
{
    CaseRecord cases[MAX_CASES];
    CategoryScore categories[MAX_CATEGORIES] = {0};
    double input_vital[FEATURE_COUNT];
    double minimum[FEATURE_COUNT], maximum[FEATURE_COUNT];
    int input_symptom[SYMPTOM_COUNT] = {0};

    int case_count = load_cases(cases, MAX_CASES);
    if (case_count <= 0) {
        fprintf(stderr, "No usable historical cases were loaded.\n");
        return 1;
    }

    read_patient_input(input_vital, input_symptom);
    get_feature_ranges(cases, case_count, minimum, maximum);

    /* Calculate normalized Euclidean distance from the input to every case. */
    for (int i = 0; i < case_count; i++) {
        double squared_distance = 0.0;
        for (int f = 0; f < FEATURE_COUNT; f++) {
            double new_value = normalize(input_vital[f], minimum[f], maximum[f]);
            double old_value = normalize(cases[i].vital[f], minimum[f], maximum[f]);
            double difference = new_value - old_value;
            squared_distance += difference * difference;
        }
        cases[i].distance = sqrt(squared_distance);
    }

    /* Sort cases from closest to farthest, then count the top-k category votes. */
    qsort(cases, (size_t)case_count, sizeof(CaseRecord), compare_distance);
    int k = case_count < K_NEIGHBORS ? case_count : K_NEIGHBORS;

    int category_count = build_categories(cases, case_count, categories);
    for (int i = 0; i < k; i++) {
        int index = find_category(categories, category_count, cases[i].category);
        if (index >= 0) categories[index].knn_votes++;
    }

    /* Average symptom similarity for each category, independently of k-NN. */
    int symptom_total = 0;
    for (int s = 0; s < SYMPTOM_COUNT; s++) symptom_total += input_symptom[s];

    if (symptom_total > 0) {
        int category_case_count[MAX_CATEGORIES] = {0};
        for (int i = 0; i < case_count; i++) {
            int index = find_category(categories, category_count, cases[i].category);
            if (index >= 0) {
                categories[index].symptom_score +=
                    symptom_similarity(input_symptom, cases[i].symptom);
                category_case_count[index]++;
            }
        }
        for (int i = 0; i < category_count; i++) {
            if (category_case_count[i] > 0) {
                categories[i].symptom_score /= category_case_count[i];
            }
        }
    }

    /* Use the two equal-weight demo signals when symptoms were entered.
       If none were entered, rely on the k-NN vote alone. */
    int best = 0;
    for (int i = 0; i < category_count; i++) {
        double knn_score = (double)categories[i].knn_votes / k;
        categories[i].combined_score = symptom_total > 0
            ? 0.5 * knn_score + 0.5 * categories[i].symptom_score
            : knn_score;
        if (categories[i].combined_score > categories[best].combined_score) best = i;
    }

    /* Specialty is read from the historical rows for the selected category. */
    const char *specialty = "General Medicine";
    const char *demo_urgency = "routine";
    for (int i = 0; i < case_count; i++) {
        if (strcmp(cases[i].category, categories[best].name) == 0) {
            specialty = cases[i].specialty;
            break;
        }
    }

    /* The nearest cases' urgency tags are shown only as synthetic demo labels. */
    int urgent_demo_votes = 0;
    for (int i = 0; i < k; i++) {
        if (strcmp(cases[i].urgency, "urgent_review_demo") == 0) urgent_demo_votes++;
    }
    if (urgent_demo_votes > k / 2) demo_urgency = "urgent_review_demo";
    else if (urgent_demo_votes > 0) demo_urgency = "priority_review";

    printf("\n--- MediSense AI demo result ---\n");
    printf("Possible case category: %s\n", categories[best].name);
    printf("k-NN vote: %d of %d nearest synthetic cases\n",
           categories[best].knn_votes, k);
    if (symptom_total > 0) {
        printf("Average symptom similarity for category: %.1f%%\n",
               categories[best].symptom_score * 100.0);
    } else {
        printf("Symptom match: not calculated (no symptoms selected)\n");
    }
    printf("Combined project match score: %.1f%%\n",
           categories[best].combined_score * 100.0);
    printf("Suggested specialty: %s\n", specialty);
    printf("Synthetic queue label from nearest cases: %s\n", demo_urgency);
    return 0;
}

