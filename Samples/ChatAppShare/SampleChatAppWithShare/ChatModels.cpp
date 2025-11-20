#include "ChatModels.h"

// Global data definitions
std::vector<Contact> contacts;
std::map<std::wstring, std::vector<std::wstring>> chatHistory;
int selectedContactIndex = -1;

void InitializeContacts()
{
    contacts = {
        {L"Alice Johnson", L"Hey, how are you?", {L"Alice: Hey, how are you?", L"You: I'm doing great, thanks!", L"Alice: That's wonderful to hear!"}, {}, L"Available", true},
        {L"Bob Smith", L"See you tomorrow!", {L"Bob: Are we still meeting tomorrow?", L"You: Yes, see you at 3 PM", L"Bob: See you tomorrow!"}, {}, L"In a meeting", true},
        {L"Carol Williams", L"Thanks for the help", {L"Carol: Could you help me with the project?", L"You: Of course! What do you need?", L"Carol: Thanks for the help"}, {}, L"Available", true},
        {L"David Brown", L"Great presentation!", {L"David: Great presentation today!", L"You: Thank you! I'm glad you liked it"}, {}, L"Away", false},
        {L"Emma Davis", L"Coffee later?", {L"Emma: Want to grab coffee later?", L"You: Sure! What time works for you?", L"Emma: Coffee later?"}, {}, L"Available", true},
        {L"Frank Miller", L"Happy Birthday!", {L"Frank: Happy Birthday!", L"You: Thank you so much!"}, {}, L"Busy", true},
        {L"Grace Wilson", L"Meeting rescheduled", {L"Grace: Meeting has been rescheduled to 4 PM", L"You: Got it, thanks for letting me know"}, {}, L"Available", true},
        {L"Henry Taylor", L"Weekend plans?", {L"Henry: Any plans for the weekend?", L"You: Nothing concrete yet", L"Henry: Weekend plans?"}, {}, L"Offline", false},
        {L"Ivy Anderson", L"Project update", {L"Ivy: Here's the project update you requested", L"You: Perfect, reviewing it now"}, {}, L"Available", true},
        {L"Jack Thompson", L"Game night Friday", {L"Jack: Game night this Friday?", L"You: Count me in!", L"Jack: Game night Friday"}, {}, L"Gaming", true},
        {L"Kate Garcia", L"Recipe sharing", {L"Kate: Loved that recipe you shared!", L"You: I'm so glad you enjoyed it!"}, {}, L"Cooking", true},
        {L"Leo Martinez", L"Workout buddy", {L"Leo: Gym session tomorrow morning?", L"You: Absolutely! 7 AM as usual?"}, {}, L"At the gym", true},
        {L"Mia Rodriguez", L"Book recommendation", {L"Mia: Any good book recommendations?", L"You: I just finished a great mystery novel"}, {}, L"Reading", true},
        {L"Noah Lee", L"Tech discussion", {L"Noah: Thoughts on the new framework?", L"You: It looks promising! Want to discuss over lunch?"}, {}, L"Coding", true},
        {L"Olivia Clark", L"Travel planning", {L"Olivia: Planning the vacation itinerary", L"You: Excited to see what you've planned!"}, {}, L"Traveling", false}
    };

    // Add some sample shared files to demonstrate the feature
    SYSTEMTIME st;
    GetSystemTime(&st);
    
    contacts[0].sharedFiles.push_back({L"Project_Proposal.docx", L"C:\\Documents\\Project_Proposal.docx", L"Alice", st});
    contacts[1].sharedFiles.push_back({L"Meeting_Notes.pdf", L"C:\\Documents\\Meeting_Notes.pdf", L"Bob", st});
    contacts[2].sharedFiles.push_back({L"Budget_Spreadsheet.xlsx", L"C:\\Documents\\Budget_Spreadsheet.xlsx", L"Carol", st});
}

Contact* GetSelectedContact()
{
    if (IsValidContactIndex(selectedContactIndex)) {
        return &contacts[selectedContactIndex];
    }
    return nullptr;
}

bool IsValidContactIndex(int index)
{
    return index >= 0 && index < (int)contacts.size();
}