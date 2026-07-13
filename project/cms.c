/* ===================================================================
   CONTACT MANAGEMENT SYSTEM
   Data Structures used:
     1. ARRAY OF STRUCTS  -> primary storage (add/edit/delete/save/load)
     2. LINKED LIST       -> built from the array on demand,
                             used to demonstrate SORTING and SEARCHING
                             via pointer traversal (not array indexing)
   File handling: binary file "contacts.dat" for persistence

   Additional features:
     - Duplicate name detection on Add/Edit
     - Phone validation: must be exactly 10 digits
     - Email validation: must match a valid user@domain.ext pattern
     - Delete supports lookup by NAME or by ID
     - MAX_CONTACTS raised to 10000 to support large-scale testing
   =================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_CONTACTS 10000
#define NAME_LEN     50
#define PHONE_LEN    15
#define EMAIL_LEN    50
#define FILENAME     "contacts.dat"

/* ---------- 1. STRUCT DEFINITION ---------- */
typedef struct {
    int  id;
    char name[NAME_LEN];
    char phone[PHONE_LEN];
    char email[EMAIL_LEN];
} Contact;

/* ---------- LINKED LIST NODE (for sort/search demo) ---------- */
typedef struct Node {
    Contact data;
    struct Node *next;
} Node;

/* ---------- GLOBALS ---------- */
Contact contacts[MAX_CONTACTS];   /* array = primary storage */
int contactCount = 0;
int nextId = 1;                   /* auto-incrementing contact ID */

/* ---------- FUNCTION PROTOTYPES ---------- */
void addContact(void);
void viewContacts(void);
void editContact(void);
void deleteContact(void);
void searchContactLinkedList(void);
void sortContactsLinkedList(void);
void saveToFile(void);
void loadFromFile(void);
int  findIndexById(int id);
void printContact(Contact c);

Node* buildLinkedList(void);
void  freeLinkedList(Node *head);
Node* sortLinkedListByName(Node *head);   /* returns new head */

void pause_enter(void);
int getValidatedInt(const char *prompt);
int strstr_ci(const char *haystack, const char *needle);

/* ---------- NEW: validation & duplicate-check helpers ---------- */
int isValidPhone(const char *phone);
int isValidEmail(const char *email);
int isDuplicateName(const char *name);
void readValidatedPhone(char *dest);
void readValidatedEmail(char *dest);
int findIndexByName(const char *name);
int strcasecmp_local(const char *a, const char *b);

/* ===================================================================
   MAIN
   =================================================================== */
int main(void) {
    int choice;

    loadFromFile();   /* load existing data at startup */

    do {
        printf("\n========= CONTACT MANAGEMENT SYSTEM =========\n");
        printf("  [Array-based storage | Linked-list sort & search]\n");
        printf("-----------------------------------------------\n");
        printf(" 1. Add Contact\n");
        printf(" 2. View All Contacts\n");
        printf(" 3. Edit Contact\n");
        printf(" 4. Delete Contact (by Name or ID)\n");
        printf(" 5. Search Contact (Linked List)\n");
        printf(" 6. Sort Contacts by Name (Linked List)\n");
        printf(" 7. Save to File\n");
        printf(" 8. Exit\n");
        printf("-----------------------------------------------\n");
        choice = getValidatedInt("Enter your choice: ");

        switch (choice) {
            case 1: addContact(); break;
            case 2: viewContacts(); break;
            case 3: editContact(); break;
            case 4: deleteContact(); break;
            case 5: searchContactLinkedList(); break;
            case 6: sortContactsLinkedList(); break;
            case 7: saveToFile(); break;
            case 8:
                saveToFile();   /* auto-save on exit */
                printf("Data saved. Exiting... Bye!\n");
                break;
            default:
                printf("Invalid choice. Try again.\n");
        }
    } while (choice != 8);

    return 0;
}

/* ===================================================================
   UTILITY FUNCTIONS
   =================================================================== */

/* Validates integer input so the menu doesn't crash on bad input */
int getValidatedInt(const char *prompt) {
    int value;
    char buffer[100];

    printf("%s", prompt);
    while (1) {
        if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
            if (sscanf(buffer, "%d", &value) == 1) {
                return value;
            }
        }
        printf("Invalid input. Please enter a number: ");
    }
}

void pause_enter(void) {
    printf("\nPress Enter to continue...");
    getchar();
}

/* Finds array index of a contact by its ID. Returns -1 if not found */
int findIndexById(int id) {
    for (int i = 0; i < contactCount; i++) {
        if (contacts[i].id == id) return i;
    }
    return -1;
}

void printContact(Contact c) {
    printf("ID: %-4d Name: %-20s Phone: %-15s Email: %-25s\n",
           c.id, c.name, c.phone, c.email);
}

/* Case-insensitive substring check: returns 1 if needle found in haystack */
int strstr_ci(const char *haystack, const char *needle) {
    char h[NAME_LEN], n[NAME_LEN];
    int i;

    for (i = 0; haystack[i] && i < NAME_LEN - 1; i++) h[i] = tolower((unsigned char)haystack[i]);
    h[i] = '\0';
    for (i = 0; needle[i] && i < NAME_LEN - 1; i++) n[i] = tolower((unsigned char)needle[i]);
    n[i] = '\0';

    return strstr(h, n) != NULL;
}

/* ===================================================================
   NEW: VALIDATION HELPERS
   =================================================================== */

/* Phone must be EXACTLY 10 digits, no letters/symbols, no more/less digits */
int isValidPhone(const char *phone) {
    int len = strlen(phone);
    if (len != 10) return 0;          /* reject anything not exactly 10 chars */

    for (int i = 0; i < len; i++) {
        if (!isdigit((unsigned char) phone[i])) return 0;   /* reject letters/symbols */
    }
    return 1;
}

/* Basic but solid email format check:
   - exactly one '@'
   - at least one character before '@'  (local part)
   - at least one '.' AFTER the '@'     (domain must have a dot)
   - no '.' immediately after '@'        (rejects "user@.com")
   - at least one character between '@' and the following '.'
   - at least one character after the final '.'  (rejects "user@domain.")
   This is deliberately a manual character-by-character check (no regex
   library), which keeps the project pure C / DSA-style, matching the
   rest of the program. */
int isValidEmail(const char *email) {
    int len = strlen(email);
    if (len < 5) return 0;   /* shortest possible valid email: a@b.co */

    int atPos = -1;
    int atCount = 0;

    /* find '@' and make sure there is exactly one */
    for (int i = 0; i < len; i++) {
        if (email[i] == '@') {
            atCount++;
            atPos = i;
        }
    }
    if (atCount != 1) return 0;          /* reject 0 or multiple '@' */
    if (atPos == 0) return 0;            /* reject "@gmail.com" - nothing before @ */
    if (atPos == len - 1) return 0;      /* reject "abc@" - nothing after @ */

    /* reject space anywhere in the email */
    for (int i = 0; i < len; i++) {
        if (isspace((unsigned char) email[i])) return 0;
    }

    /* domain part is everything after '@' */
    const char *domain = email + atPos + 1;
    int domainLen = strlen(domain);

    if (domain[0] == '.') return 0;      /* reject "user@.com" */

    /* find the LAST '.' in the domain (so "abc@.com" already rejected above,
       and "abc.def@mail.co.in" style domains still work) */
    int dotPos = -1;
    for (int i = 0; i < domainLen; i++) {
        if (domain[i] == '.') dotPos = i;
    }
    if (dotPos == -1) return 0;                 /* reject "abc@gmail" - no dot at all */
    if (dotPos == domainLen - 1) return 0;       /* reject "abc@domain." - nothing after last dot */
    if (dotPos == 0) return 0;                   /* reject "abc@.com" (redundant safety check) */

    /* extension after the last dot must be letters only and at least 2 chars
       (covers .com / .in / .co.in / .org etc.) */
    int extLen = domainLen - dotPos - 1;
    if (extLen < 2) return 0;
    for (int i = dotPos + 1; i < domainLen; i++) {
        if (!isalpha((unsigned char) domain[i])) return 0;
    }

    return 1;   /* passed all checks */
}

/* TC_004: returns 1 if a contact with this exact name (case-insensitive)
   already exists in the array, 0 otherwise */
int isDuplicateName(const char *name) {
    for (int i = 0; i < contactCount; i++) {
        if (strcasecmp_local(contacts[i].name, name) == 0) {
            return 1;
        }
    }
    return 0;
}

/* Portable case-insensitive exact string comparison (plain C does not
   guarantee POSIX strcasecmp, so this is written manually). Returns 0
   when the two strings are equal ignoring case. */
int strcasecmp_local(const char *a, const char *b) {
    while (*a && *b) {
        char ca = (char) tolower((unsigned char) *a);
        char cb = (char) tolower((unsigned char) *b);
        if (ca != cb) return (ca < cb) ? -1 : 1;
        a++;
        b++;
    }
    return (*a == *b) ? 0 : ((*a) ? 1 : -1);
}

/* Finds array index of a contact by exact name (case-insensitive).
   Returns -1 if not found. Used by the improved delete-by-name feature. */
int findIndexByName(const char *name) {
    for (int i = 0; i < contactCount; i++) {
        if (strcasecmp_local(contacts[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

/* Keeps asking until the user enters a valid 10-digit phone number */
void readValidatedPhone(char *dest) {
    char buffer[60];
    while (1) {
        printf("Enter phone (exactly 10 digits): ");
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = '\0';

        if (isValidPhone(buffer)) {
            strcpy(dest, buffer);
            return;
        }
        printf("Invalid phone number. Must be exactly 10 digits, no letters or symbols.\n");
    }
}

/* Keeps asking until the user enters a syntactically valid email */
void readValidatedEmail(char *dest) {
    char buffer[60];
    while (1) {
        printf("Enter email (e.g. user@gmail.com): ");
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = '\0';

        if (isValidEmail(buffer)) {
            strcpy(dest, buffer);
            return;
        }
        printf("Invalid email format. Must look like user@domain.com\n");
    }
}

/* ===================================================================
   2. ARRAY-BASED CRUD OPERATIONS
   =================================================================== */

void addContact(void) {
    if (contactCount >= MAX_CONTACTS) {
        printf("Contact list is full!\n");
        return;
    }

    char nameBuf[NAME_LEN];

    printf("\n--- Add New Contact ---\n");
    printf("Enter name: ");
    fgets(nameBuf, NAME_LEN, stdin);
    nameBuf[strcspn(nameBuf, "\n")] = '\0';   /* strip newline */

    /* Requirement 2: reject duplicate names BEFORE asking for phone/email,
       so the user doesn't waste time entering details that get discarded */
    if (isDuplicateName(nameBuf)) {
        printf("Duplicate contact detected.\n");
        return;
    }

    Contact c;
    memset(&c, 0, sizeof(Contact));   /* zero out so no leftover/garbage bytes in saved file */
    c.id = nextId++;
    strcpy(c.name, nameBuf);

    /* Requirement 3: phone must be exactly 10 digits - keeps re-prompting until valid */
    readValidatedPhone(c.phone);

    /* Requirement 4: email must match a valid format - keeps re-prompting until valid */
    readValidatedEmail(c.email);

    contacts[contactCount++] = c;
    printf("Contact added successfully!\n");
}

void viewContacts(void) {
    printf("\n--- All Contacts (%d total) ---\n", contactCount);
    if (contactCount == 0) {
        printf("No contacts found.\n");
        return;
    }
    for (int i = 0; i < contactCount; i++) {
        printContact(contacts[i]);
    }
}

void editContact(void) {
    if (contactCount == 0) {
        printf("No contacts to edit.\n");
        return;
    }

    int id = getValidatedInt("Enter contact ID to edit: ");
    int idx = findIndexById(id);

    if (idx == -1) {
        printf("Contact with ID %d not found.\n", id);
        return;
    }

    printf("Editing contact:\n");
    printContact(contacts[idx]);

    char buffer[60];

    printf("Enter new name (leave blank to keep current): ");
    fgets(buffer, NAME_LEN, stdin);
    buffer[strcspn(buffer, "\n")] = '\0';
    if (strlen(buffer) > 0) {
        /* Requirement 2: don't let an edit create a duplicate name either.
           (Skip the check if they're just re-typing their own current name.) */
        if (strcasecmp_local(buffer, contacts[idx].name) != 0 && isDuplicateName(buffer)) {
            printf("Duplicate contact detected. Name not changed.\n");
        } else {
            strcpy(contacts[idx].name, buffer);
        }
    }

    printf("Enter new phone (leave blank to keep current, or type 10 digits): ");
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = '\0';
    if (strlen(buffer) > 0) {
        if (isValidPhone(buffer)) {
            strcpy(contacts[idx].phone, buffer);
        } else {
            printf("Invalid phone number (must be exactly 10 digits). Phone not changed.\n");
        }
    }

    printf("Enter new email (leave blank to keep current): ");
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = '\0';
    if (strlen(buffer) > 0) {
        if (isValidEmail(buffer)) {
            strcpy(contacts[idx].email, buffer);
        } else {
            printf("Invalid email format. Email not changed.\n");
        }
    }

    printf("Contact updated successfully!\n");
}

/* Requirement 6: delete works by EITHER name or ID, user picks which.
   Internally both paths end up at the same array index, then do the
   same left-shift to close the gap - the well-known "delete from middle
   of an array" technique. */
void deleteContact(void) {
    if (contactCount == 0) {
        printf("No contacts to delete.\n");
        return;
    }

    int mode = getValidatedInt("Delete by (1) Name or (2) ID? Enter 1 or 2: ");
    int idx = -1;

    if (mode == 1) {
        char nameBuf[NAME_LEN];
        printf("Enter name to delete: ");
        fgets(nameBuf, NAME_LEN, stdin);
        nameBuf[strcspn(nameBuf, "\n")] = '\0';
        idx = findIndexByName(nameBuf);

        if (idx == -1) {
            printf("Not found. No contact named \"%s\" exists.\n", nameBuf);
            return;
        }
    } else if (mode == 2) {
        int id = getValidatedInt("Enter contact ID to delete: ");
        idx = findIndexById(id);

        if (idx == -1) {
            printf("Not found. No contact with ID %d exists.\n", id);
            return;
        }
    } else {
        printf("Invalid option. Please choose 1 (Name) or 2 (ID).\n");
        return;
    }

    printf("Deleting: ");
    printContact(contacts[idx]);

    /* shift all elements after idx one step left to close the gap */
    for (int i = idx; i < contactCount - 1; i++) {
        contacts[i] = contacts[i + 1];
    }
    contactCount--;

    printf("Contact deleted successfully!\n");
}

/* ===================================================================
   3. LINKED LIST OPERATIONS (built from array, used for sort & search)
   =================================================================== */

/* Builds a linked list from the current array contents */
Node* buildLinkedList(void) {
    Node *head = NULL, *tail = NULL;

    for (int i = 0; i < contactCount; i++) {
        Node *newNode = (Node*) malloc(sizeof(Node));
        if (!newNode) {
            printf("Memory allocation failed!\n");
            exit(1);
        }
        newNode->data = contacts[i];
        newNode->next = NULL;

        if (head == NULL) {
            head = newNode;
            tail = newNode;
        } else {
            tail->next = newNode;
            tail = newNode;
        }
    }
    return head;
}

void freeLinkedList(Node *head) {
    Node *temp;
    while (head != NULL) {
        temp = head;
        head = head->next;
        free(temp);
    }
}

/* Simple search by name traversing the linked list */
void searchContactLinkedList(void) {
    if (contactCount == 0) {
        printf("No contacts to search.\n");
        return;
    }

    char searchName[NAME_LEN];
    printf("Enter name to search: ");
    fgets(searchName, NAME_LEN, stdin);
    searchName[strcspn(searchName, "\n")] = '\0';

    Node *head = buildLinkedList();
    Node *curr = head;
    int found = 0;

    printf("\n--- Search Results ---\n");
    while (curr != NULL) {
        /* case-insensitive partial match */
        if (strstr_ci(curr->data.name, searchName)) {
            printContact(curr->data);
            found = 1;
        }
        curr = curr->next;
    }

    if (!found) {
        printf("Not found. No contact matching \"%s\" exists.\n", searchName);
    }

    freeLinkedList(head);
}

/* Merge sort on linked list, sorting by name (classic DS technique) */
Node* mergeSortedLists(Node *a, Node *b) {
    if (!a) return b;
    if (!b) return a;

    if (strcmp(a->data.name, b->data.name) <= 0) {
        a->next = mergeSortedLists(a->next, b);
        return a;
    } else {
        b->next = mergeSortedLists(a, b->next);
        return b;
    }
}

Node* getMiddleAndSplit(Node *head) {
    /* splits list into two halves, returns head of second half */
    if (!head || !head->next) return NULL;

    Node *slow = head, *fast = head->next;
    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
    }
    Node *secondHalf = slow->next;
    slow->next = NULL;
    return secondHalf;
}

Node* sortLinkedListByName(Node *head) {
    if (!head || !head->next) return head;

    Node *secondHalf = getMiddleAndSplit(head);
    Node *left  = sortLinkedListByName(head);
    Node *right = sortLinkedListByName(secondHalf);

    return mergeSortedLists(left, right);
}

void sortContactsLinkedList(void) {
    if (contactCount == 0) {
        printf("No contacts to sort.\n");
        return;
    }

    Node *head = buildLinkedList();
    head = sortLinkedListByName(head);

    printf("\n--- Contacts Sorted by Name (via Linked List Merge Sort) ---\n");
    Node *curr = head;
    int i = 0;
    while (curr != NULL) {
        printContact(curr->data);
        contacts[i++] = curr->data;   /* write sorted order back to array */
        curr = curr->next;
    }

    printf("\nArray storage has been updated to reflect this sorted order.\n");
    freeLinkedList(head);
}

/* ===================================================================
   4. FILE HANDLING (persistence)
   =================================================================== */

void saveToFile(void) {
    FILE *fp = fopen(FILENAME, "wb");
    if (!fp) {
        printf("Error: could not open file for saving.\n");
        return;
    }

    fwrite(&contactCount, sizeof(int), 1, fp);
    fwrite(&nextId, sizeof(int), 1, fp);
    fwrite(contacts, sizeof(Contact), contactCount, fp);

    fclose(fp);
    printf("Saved %d contact(s) to %s\n", contactCount, FILENAME);
}

void loadFromFile(void) {
    FILE *fp = fopen(FILENAME, "rb");
    
    if (!fp) {
        /* no existing file yet - that's fine on first run */
        return;
    }

    fread(&contactCount, sizeof(int), 1, fp);
    fread(&nextId, sizeof(int), 1, fp);
    fread(contacts, sizeof(Contact), contactCount, fp);

    fclose(fp);
    printf("Loaded %d contact(s) from %s\n", contactCount, FILENAME);
}

