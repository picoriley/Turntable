#include "Game/UserData/UserProfile.hpp"
#include <math.h>
#include "Engine/Input/Console.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "AchievementManager.hpp"
#include "Engine/Core/Events/EventSystem.hpp"
#include "ThirdParty/Parsers/XMLParser.hpp"
#include "Engine/Input/XMLUtils.hpp"
#include "Engine/Input/InputOutputUtils.hpp"

//-----------------------------------------------------------------------------------
UserProfile::UserProfile()
{

}

//-----------------------------------------------------------------------------------
UserProfile::~UserProfile()
{
    SaveToDisk();
}

//-----------------------------------------------------------------------------------
void UserProfile::AddExperience(unsigned int expGained)
{
    m_experience += expGained;
    if (CheckForLevelUp())
    {
        unsigned int newLevel = CalculateLevelFromExperience(m_experience);
        m_numTokens += newLevel - m_level;
        m_level = newLevel;
        EventSystem::FireEvent("LevelUp");
    }
}

//-----------------------------------------------------------------------------------
bool UserProfile::CheckForLevelUp()
{
    return (m_experience >= CalculateExperienceRequiredForLevel(m_level + 1));
}

//-----------------------------------------------------------------------------------
unsigned int UserProfile::CalculateExperienceRequiredForLevel(unsigned int level)
{
    level -= 1; //Offset so that level 1 takes 0 experience.
    float levelOverConstant = (float)level / EXPERIENCE_CURVE_CONSTANT;
    float levelOverConstantSquared = levelOverConstant * levelOverConstant;
    return (unsigned int)ceilf(levelOverConstantSquared);
}

//-----------------------------------------------------------------------------------
void UserProfile::SaveToDisk(const std::string& profileName /*= "Default"*/)
{
    XMLNode root = XMLNode::createXMLTopNode("UserProfile");
    root.addAttribute("Name", profileName.c_str()); //Todo: Add support for multiple profiles.
    root.addAttribute("Version", USER_PROFILE_VERSION_STRING); //Todo: Add support for multiple profiles.
    XMLNode expNode = root.addChild("Stats");
    expNode.addAttribute("Exp", Stringf("%i", m_experience).c_str());
    expNode.addAttribute("Level", Stringf("%i", m_level).c_str());
    expNode.addAttribute("Tokens", Stringf("%i", m_numTokens).c_str());

    std::string saveDirectory = GetAppDataDirectory();
    EnsureDirectoryExists(saveDirectory + "\\Turntable");
    EnsureDirectoryExists(saveDirectory + "\\Turntable\\UserProfiles");
    std::string fullPath = saveDirectory + "\\Turntable\\UserProfiles\\" + profileName + ".xml";
    root.writeToFile(fullPath.c_str());
}

//-----------------------------------------------------------------------------------
UserProfile* UserProfile::LoadFromDisk(const std::string& profileName /*= "Default"*/)
{
    std::string saveDirectory = GetAppDataDirectory();
    std::string fullPath = saveDirectory + "\\Turntable\\UserProfiles\\" + profileName + ".xml";
    if (!FileExists(fullPath))
    {
        return nullptr;
    }

    UserProfile* loadedProfile = new UserProfile();

    XMLNode root = XMLUtils::OpenXMLDocument(fullPath);
    //TODO: check for version attribute and multiple profile names.
    XMLNode expNode = XMLUtils::GetChildNodeAtPosition(root, "UserProfile").getChildNode("Stats");
    loadedProfile->m_experience = std::stoi(expNode.getAttribute("Exp"));
    loadedProfile->m_level = std::stoi(expNode.getAttribute("Level"));
    loadedProfile->m_numTokens = std::stoi(expNode.getAttribute("Tokens"));

    return loadedProfile;
}

//-----------------------------------------------------------------------------------
unsigned int UserProfile::CalculateLevelFromExperience(unsigned int experience)
{
    //Offset level by one so that level 1 takes 0 experience.
    return (EXPERIENCE_CURVE_CONSTANT * sqrt(experience)) + 1;
}

//CONSOLE COMMANDS/////////////////////////////////////////////////////////////////////
CONSOLE_COMMAND(stats)
{
    UNUSED(args);
    Console::instance->PrintLine(Stringf("You are level %i.", AchievementManager::instance->m_currentProfile->m_level), RGBA::CYAN);
    Console::instance->PrintLine(Stringf("You have %i experience.", AchievementManager::instance->m_currentProfile->m_experience), RGBA::CERULEAN);
    Console::instance->PrintLine(Stringf("You have %i tokens available to spend.", AchievementManager::instance->m_currentProfile->m_numTokens), RGBA::BADDAD);
}

CONSOLE_COMMAND(addexp)
{
    if (!args.HasArgs(1))
    {
        Console::instance->PrintLine("addexp <number of experience points to add>", RGBA::RED);
        return;
    }

    AchievementManager::instance->m_currentProfile->AddExperience(args.GetIntArgument(0));
    Console::instance->PrintLine(Stringf("You are now level %i.", AchievementManager::instance->m_currentProfile->m_level), RGBA::CYAN);
    Console::instance->PrintLine(Stringf("You now have %i experience.", AchievementManager::instance->m_currentProfile->m_experience), RGBA::CERULEAN);
    Console::instance->PrintLine(Stringf("You now have %i tokens available to spend.", AchievementManager::instance->m_currentProfile->m_numTokens), RGBA::BADDAD);
}

CONSOLE_COMMAND(printlevels)
{
    for (int i = 1; i < 100; ++i)
    {
        Console::instance->PrintLine(Stringf("Level %i takes %i exp.", i, AchievementManager::instance->m_currentProfile->CalculateExperienceRequiredForLevel(i)), RGBA::BADDAD);
    }
}