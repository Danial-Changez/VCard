// Name: Danial Changez
// Student #: 1232341
// Class: CIS*2750

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "../include/VCParser.h"
#include "../include/LinkedListAPI.h"

//------------------------------
//     My Helper Functions
//------------------------------

// duplicateString
// Allocates memory and returns a duplicate of the input string.
char *duplicateString(const char *str)
{
    if (!str)
        return NULL;
    char *newStr = malloc(strlen(str) + 1);
    if (newStr)
        strcpy(newStr, str);
    return newStr;
}

// trimWhitespace
// Removes any leading and trailing whitespace from the string (modifies in place).
char *trimWhitespace(char *str)
{
    if (!str)
        return NULL;
    while (*str && isspace((unsigned char)*str))
        str++;
    if (*str == '\0')
        return str;
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;
    end[1] = '\0';
    return str;
}

// containsAlpha
// Returns true if the input string contains at least one alphabetic character.
bool containsAlpha(const char *str)
{
    for (size_t i = 0; i < strlen(str); i++)
    {
        if (isalpha((unsigned char)str[i]))
            return true;
    }
    return false;
}

// splitComposite
// Splits a composite property value (e.g. the N property) on the specified delimiter,
// preserving empty tokens. Returns a pointer to a newly allocated List of strings.
List *splitComposite(const char *str, char delim)
{
    List *list = initializeList(&valueToString, &deleteValue, &compareValues);
    const char *start = str;
    const char *p = str;
    while (1)
    {
        if (*p == delim || *p == '\0')
        {
            size_t len = p - start;
            char *token = malloc(len + 1);
            if (token)
            {
                strncpy(token, start, len);
                token[len] = '\0';
                insertBack(list, token);
            }
            if (*p == '\0')
                break;
            start = p + 1;
        }
        p++;
    }
    return list;
}

//------------------------------
//      Main Parser Functions
//------------------------------

VCardErrorCode createCard(char *fileName, Card **obj)
{
    if (!fileName || strlen(fileName) == 0 || !obj)
        return INV_FILE;

    char *ext = strrchr(fileName, '.');
    if (!ext || (strcmp(ext, ".vcf") != 0 && strcmp(ext, ".vcard") != 0))
        return INV_FILE;

    FILE *file = fopen(fileName, "r");
    if (!file)
        return INV_FILE;

    Card *newCard = malloc(sizeof(Card));
    if (!newCard)
    {
        fclose(file);
        return OTHER_ERROR;
    }
    newCard->fn = NULL;
    newCard->optionalProperties = initializeList(&propertyToString, &deleteProperty, &compareProperties);
    newCard->birthday = NULL;
    newCard->anniversary = NULL;

    char buffer[256];
    int numLines = 0, capacity = 10;
    char **logicalLines = malloc(capacity * sizeof(char *));
    if (!logicalLines)
    {
        fclose(file);
        deleteCard(newCard);
        return OTHER_ERROR;
    }
    char *currentLogical = NULL;
    while (fgets(buffer, sizeof(buffer), file))
    {
        size_t len = strlen(buffer);
        if (len < 2 || buffer[len - 1] != '\n' || buffer[len - 2] != '\r')
        {
            for (int i = 0; i < numLines; i++)
                free(logicalLines[i]);
            free(logicalLines);
            if (currentLogical)
                free(currentLogical);
            fclose(file);
            deleteCard(newCard);
            return INV_CARD;
        }
        buffer[len - 2] = '\0';
        if (buffer[0] == ' ' || buffer[0] == '\t')
        {
            if (!currentLogical)
            {
                for (int i = 0; i < numLines; i++)
                    free(logicalLines[i]);
                free(logicalLines);
                fclose(file);
                deleteCard(newCard);
                return INV_PROP;
            }
            char *trimmed = buffer;
            while (*trimmed && (*trimmed == ' ' || *trimmed == '\t'))
                trimmed++;
            size_t newSize = strlen(currentLogical) + strlen(trimmed) + 1;
            char *tmp = realloc(currentLogical, newSize);
            if (!tmp)
            {
                for (int i = 0; i < numLines; i++)
                    free(logicalLines[i]);
                free(logicalLines);
                fclose(file);
                deleteCard(newCard);
                return OTHER_ERROR;
            }
            currentLogical = tmp;
            strcat(currentLogical, trimmed);
        }
        else
        {
            if (currentLogical)
            {
                if (numLines == capacity)
                {
                    capacity *= 2;
                    char **tmp = realloc(logicalLines, capacity * sizeof(char *));
                    if (!tmp)
                    {
                        for (int i = 0; i < numLines; i++)
                            free(logicalLines[i]);
                        free(logicalLines);
                        free(currentLogical);
                        fclose(file);
                        deleteCard(newCard);
                        return OTHER_ERROR;
                    }
                    logicalLines = tmp;
                }
                logicalLines[numLines++] = currentLogical;
            }
            currentLogical = duplicateString(buffer);
            if (!currentLogical)
            {
                for (int i = 0; i < numLines; i++)
                    free(logicalLines[i]);
                free(logicalLines);
                fclose(file);
                deleteCard(newCard);
                return OTHER_ERROR;
            }
        }
    }
    if (currentLogical)
    {
        if (numLines == capacity)
        {
            capacity *= 2;
            char **tmp = realloc(logicalLines, capacity * sizeof(char *));
            if (!tmp)
            {
                for (int i = 0; i < numLines; i++)
                    free(logicalLines[i]);
                free(logicalLines);
                free(currentLogical);
                fclose(file);
                deleteCard(newCard);
                return OTHER_ERROR;
            }
            logicalLines = tmp;
        }
        logicalLines[numLines++] = currentLogical;
    }
    fclose(file);

    if (numLines < 2 ||
        strcmp(logicalLines[0], "BEGIN:VCARD") != 0 ||
        strcmp(logicalLines[numLines - 1], "END:VCARD") != 0)
    {
        for (int i = 0; i < numLines; i++)
            free(logicalLines[i]);
        free(logicalLines);
        deleteCard(newCard);
        return INV_CARD;
    }

    bool versionFound = false;
    for (int i = 1; i < numLines - 1; i++)
    {
        char *line = logicalLines[i];
        if (strlen(line) == 0)
            continue;
        char *colon = strchr(line, ':');
        if (!colon)
        {
            for (int j = 0; j < numLines; j++)
                free(logicalLines[j]);
            free(logicalLines);
            deleteCard(newCard);
            return INV_PROP;
        }
        *colon = '\0';
        char *leftPart = trimWhitespace(line);
        char *rightPart = trimWhitespace(colon + 1);
        if (strlen(rightPart) == 0)
        {
            for (int j = 0; j < numLines; j++)
                free(logicalLines[j]);
            free(logicalLines);
            deleteCard(newCard);
            return INV_PROP;
        }

        char *leftDup = duplicateString(leftPart);
        if (!leftDup)
        {
            for (int j = 0; j < numLines; j++)
                free(logicalLines[j]);
            free(logicalLines);
            deleteCard(newCard);
            return OTHER_ERROR;
        }
        char *token = strtok(leftDup, ";");
        if (!token)
        {
            free(leftDup);
            for (int j = 0; j < numLines; j++)
                free(logicalLines[j]);
            free(logicalLines);
            deleteCard(newCard);
            return INV_PROP;
        }
        token = trimWhitespace(token);
        char *propGroup = NULL;
        char *propName = NULL;
        char *dot = strchr(token, '.');
        if (dot)
        {
            *dot = '\0';
            propGroup = duplicateString(token);
            propName = duplicateString(trimWhitespace(dot + 1));
        }
        else
        {
            propGroup = duplicateString("");
            propName = duplicateString(token);
        }
        free(leftDup);
        if (!propGroup || !propName || strlen(propName) == 0)
        {
            free(propGroup);
            free(propName);
            for (int j = 0; j < numLines; j++)
                free(logicalLines[j]);
            free(logicalLines);
            deleteCard(newCard);
            return INV_PROP;
        }

        Property *property = malloc(sizeof(Property));
        if (!property)
        {
            free(propGroup);
            free(propName);
            for (int j = 0; j < numLines; j++)
                free(logicalLines[j]);
            free(logicalLines);
            deleteCard(newCard);
            return OTHER_ERROR;
        }
        property->name = propName;
        property->group = propGroup;
        property->parameters = initializeList(&parameterToString, &deleteParameter, &compareParameters);
        property->values = initializeList(&valueToString, &deleteValue, &compareValues);

        while ((token = strtok(NULL, ";")) != NULL)
        {
            token = trimWhitespace(token);
            char *equalSign = strchr(token, '=');
            if (!equalSign)
            {
                deleteProperty(property);
                for (int j = 0; j < numLines; j++)
                    free(logicalLines[j]);
                free(logicalLines);
                deleteCard(newCard);
                return INV_PROP;
            }
            *equalSign = '\0';
            char *paramName = duplicateString(trimWhitespace(token));
            char *paramValue = duplicateString(trimWhitespace(equalSign + 1));
            if (!paramName || !paramValue || strlen(paramName) == 0 || strlen(paramValue) == 0)
            {
                free(paramName);
                free(paramValue);
                deleteProperty(property);
                for (int j = 0; j < numLines; j++)
                    free(logicalLines[j]);
                free(logicalLines);
                deleteCard(newCard);
                return INV_PROP;
            }
            Parameter *param = malloc(sizeof(Parameter));
            if (!param)
            {
                free(paramName);
                free(paramValue);
                deleteProperty(property);
                for (int j = 0; j < numLines; j++)
                    free(logicalLines[j]);
                free(logicalLines);
                deleteCard(newCard);
                return OTHER_ERROR;
            }
            param->name = paramName;
            param->value = paramValue;
            insertBack(property->parameters, param);
        }

        if (strcmp(property->name, "N") == 0)
        {
            clearList(property->values);
            free(property->values);
            property->values = splitComposite(rightPart, ';');
        }
        else
        {
            char *val = duplicateString(rightPart);
            if (strcmp(property->name, "TEL") == 0)
            {
                char *semi = strchr(val, ';');
                if (semi)
                    *semi = '\0';
            }
            insertBack(property->values, val);
        }

        if (strcmp(property->name, "BEGIN") == 0 ||
            strcmp(property->name, "END") == 0)
        {
            deleteProperty(property);
        }
        else if (strcmp(property->name, "VERSION") == 0)
        {
            char *versionVal = (char *)getFromFront(property->values);
            if (!versionVal || strcmp(versionVal, "4.0") != 0)
            {
                deleteProperty(property);
                for (int j = 0; j < numLines; j++)
                    free(logicalLines[j]);
                free(logicalLines);
                deleteCard(newCard);
                return INV_CARD;
            }
            versionFound = true;
            deleteProperty(property);
        }
        else if (strcmp(property->name, "FN") == 0)
        {
            if (newCard->fn != NULL)
                insertBack(newCard->optionalProperties, property);
            else
                newCard->fn = property;
        }
        else if (strcmp(property->name, "BDAY") == 0)
        {
            DateTime *dt = malloc(sizeof(DateTime));
            if (!dt)
            {
                deleteProperty(property);
                for (int j = 0; j < numLines; j++)
                    free(logicalLines[j]);
                free(logicalLines);
                deleteCard(newCard);
                return OTHER_ERROR;
            }
            dt->UTC = false;
            bool isTextParam = false;
            ListIterator paramIter = createIterator(property->parameters);
            Parameter *currParam = NULL;
            while ((currParam = nextElement(&paramIter)) != NULL)
            {
                if (strcmp(currParam->name, "VALUE") == 0 &&
                    strcmp(currParam->value, "text") == 0)
                {
                    isTextParam = true;
                    break;
                }
            }
            if (isTextParam)
            {
                dt->date = duplicateString("");
                dt->time = duplicateString("");
                dt->isText = true;
                dt->text = duplicateString(rightPart);
            }
            else
            {
                char *tPos = strchr(rightPart, 'T');
                if (tPos)
                {
                    size_t dateLen = tPos - rightPart;
                    dt->date = malloc(dateLen + 1);
                    if (!dt->date)
                    {
                        free(dt);
                        deleteProperty(property);
                        for (int j = 0; j < numLines; j++)
                            free(logicalLines[j]);
                        free(logicalLines);
                        deleteCard(newCard);
                        return OTHER_ERROR;
                    }
                    strncpy(dt->date, rightPart, dateLen);
                    dt->date[dateLen] = '\0';
                    dt->time = duplicateString(tPos + 1);
                    dt->isText = false;
                    dt->text = duplicateString("");
                }
                else if (strlen(rightPart) == 10 || !containsAlpha(rightPart))
                {
                    dt->date = duplicateString(rightPart);
                    dt->time = duplicateString("");
                    dt->isText = false;
                    dt->text = duplicateString("");
                }
                else
                {
                    dt->date = duplicateString("");
                    dt->time = duplicateString("");
                    dt->isText = true;
                    dt->text = duplicateString(rightPart);
                }
            }
            newCard->birthday = dt;
            deleteProperty(property);
        }
        else if (strcmp(property->name, "ANNIVERSARY") == 0)
        {
            DateTime *dt = malloc(sizeof(DateTime));
            if (!dt)
            {
                deleteProperty(property);
                for (int j = 0; j < numLines; j++)
                    free(logicalLines[j]);
                free(logicalLines);
                deleteCard(newCard);
                return OTHER_ERROR;
            }
            dt->UTC = false;
            bool isTextParam = false;
            ListIterator paramIter = createIterator(property->parameters);
            Parameter *currParam = NULL;
            while ((currParam = nextElement(&paramIter)) != NULL)
            {
                if (strcmp(currParam->name, "VALUE") == 0 &&
                    strcmp(currParam->value, "text") == 0)
                {
                    isTextParam = true;
                    break;
                }
            }
            if (isTextParam)
            {
                dt->date = duplicateString("");
                dt->time = duplicateString("");
                dt->isText = true;
                dt->text = duplicateString(rightPart);
            }
            else
            {
                char *tPos = strchr(rightPart, 'T');
                if (tPos)
                {
                    size_t dateLen = tPos - rightPart;
                    dt->date = malloc(dateLen + 1);
                    if (!dt->date)
                    {
                        free(dt);
                        deleteProperty(property);
                        for (int j = 0; j < numLines; j++)
                            free(logicalLines[j]);
                        free(logicalLines);
                        deleteCard(newCard);
                        return OTHER_ERROR;
                    }
                    strncpy(dt->date, rightPart, dateLen);
                    dt->date[dateLen] = '\0';
                    dt->time = duplicateString(tPos + 1);
                    dt->isText = false;
                    dt->text = duplicateString("");
                }
                else if (strlen(rightPart) == 10 || !containsAlpha(rightPart))
                {
                    dt->date = duplicateString(rightPart);
                    dt->time = duplicateString("");
                    dt->isText = false;
                    dt->text = duplicateString("");
                }
                else
                {
                    dt->date = duplicateString("");
                    dt->time = duplicateString("");
                    dt->isText = true;
                    dt->text = duplicateString(rightPart);
                }
            }
            newCard->anniversary = dt;
            deleteProperty(property);
        }
        else
        {
            // All other properties go into optionalProperties.
            insertBack(newCard->optionalProperties, property);
        }
    }

    for (int i = 0; i < numLines; i++)
        free(logicalLines[i]);
    free(logicalLines);

    if (!versionFound || !newCard->fn)
    {
        deleteCard(newCard);
        return INV_CARD;
    }

    *obj = newCard;
    return OK;
}

//------------------------------
//      Module 2 Functions
//------------------------------

// propertyToFileString
// Converts a Property structure to a string in vCard file format.
// Format: [group.]name[;paramName=paramValue...]:value[;value2...]
static char *propertyToFileString(void *prop)
{
    Property *p = (Property *)prop;
    char *result = malloc(1024);
    if (!result)
        return NULL;
    result[0] = '\0';

    if (strlen(p->group) > 0)
    {
        strcat(result, p->group);
        strcat(result, ".");
    }
    strcat(result, p->name);

    // Append parameters.
    ListIterator paramIter = createIterator(p->parameters);
    void *pdata;
    while ((pdata = nextElement(&paramIter)) != NULL)
    {
        Parameter *param = (Parameter *)pdata;
        strcat(result, ";");
        strcat(result, param->name);
        strcat(result, "=");
        strcat(result, param->value);
    }

    strcat(result, ":");
    // Append property values, joined by semicolons.
    if (getLength(p->values) > 0)
    {
        ListIterator valIter = createIterator(p->values);
        void *vdata;
        bool first = true;
        while ((vdata = nextElement(&valIter)) != NULL)
        {
            if (!first)
                strcat(result, ";");
            strcat(result, (char *)vdata);
            first = false;
        }
    }
    return result;
}

// writeCard
// Writes a Card object to a file in valid vCard format with CRLF line endings.
// Output is unfolded. Returns WRITE_ERROR if any write fails; otherwise, OK.
// If fileName or obj is NULL, returns WRITE_ERROR.
VCardErrorCode writeCard(const char *fileName, const Card *obj)
{
    if (!fileName || !obj)
        return WRITE_ERROR;

    FILE *fp = fopen(fileName, "w");
    if (!fp)
        return WRITE_ERROR;

    if (fprintf(fp, "BEGIN:VCARD\r\n") < 0 ||
        fprintf(fp, "VERSION:4.0\r\n") < 0)
    {
        fclose(fp);
        return WRITE_ERROR;
    }

    // Write FN property.
    char *line = propertyToFileString((void *)obj->fn);
    if (!line)
    {
        fclose(fp);
        return WRITE_ERROR;
    }
    if (fprintf(fp, "%s\r\n", line) < 0)
    {
        free(line);
        fclose(fp);
        return WRITE_ERROR;
    }
    free(line);

    // Write BDAY property.
    if (obj->birthday)
    {
        DateTime *dt = obj->birthday;
        if (dt->isText)
        {
            if (fprintf(fp, "BDAY;VALUE=text:%s\r\n", dt->text) < 0)
            {
                fclose(fp);
                return WRITE_ERROR;
            }
        }
        else
        {
            char *bday = dateToString((void *)dt);
            if (fprintf(fp, "BDAY:%s\r\n", bday) < 0)
            {
                free(bday);
                fclose(fp);
                return WRITE_ERROR;
            }
            free(bday);
        }
    }

    // Write ANNIVERSARY property.
    if (obj->anniversary)
    {
        DateTime *dt = obj->anniversary;
        if (dt->isText)
        {
            if (fprintf(fp, "ANNIVERSARY;VALUE=text:%s\r\n", dt->text) < 0)
            {
                fclose(fp);
                return WRITE_ERROR;
            }
        }
        else
        {
            char *anniv = dateToString((void *)dt);
            if (fprintf(fp, "ANNIVERSARY:%s\r\n", anniv) < 0)
            {
                free(anniv);
                fclose(fp);
                return WRITE_ERROR;
            }
            free(anniv);
        }
    }

    // Write optional properties using propertyToFileString.
    ListIterator iter = createIterator(obj->optionalProperties);
    void *data;
    while ((data = nextElement(&iter)) != NULL)
    {
        char *propLine = propertyToFileString(data);
        if (!propLine)
        {
            fclose(fp);
            return WRITE_ERROR;
        }
        if (fprintf(fp, "%s\r\n", propLine) < 0)
        {
            free(propLine);
            fclose(fp);
            return WRITE_ERROR;
        }
        free(propLine);
    }

    if (fprintf(fp, "END:VCARD\r\n") < 0)
    {
        fclose(fp);
        return WRITE_ERROR;
    }

    if (ferror(fp))
    {
        fclose(fp);
        return WRITE_ERROR;
    }

    fclose(fp);
    return OK;
}

// validateCard
// Validates a Card object against internal structure requirements and a subset
// of the vCard format rules. Returns OK if the Card is valid; otherwise returns an
// appropriate error code: INV_CARD if the Card is invalid, INV_PROP for property-level errors,
// or INV_DT for inconsistent DateTime fields.
VCardErrorCode validateCard(const Card *obj)
{
    if (!obj)
        return INV_CARD;

    if (!obj->fn || !obj->optionalProperties)
        return INV_CARD;
    if (strcmp(obj->fn->name, "FN") != 0)
        return INV_PROP;
    if (getLength(obj->fn->values) == 0)
        return INV_PROP;

    ListIterator iter = createIterator(obj->optionalProperties);
    void *data;
    int countN = 0, countKIND = 0;
    while ((data = nextElement(&iter)) != NULL)
    {
        Property *prop = (Property *)data;
        if (strcmp(prop->name, "VERSION") == 0)
            return INV_CARD;
        if (strcmp(prop->name, "BDAY") == 0 || strcmp(prop->name, "ANNIVERSARY") == 0)
            return INV_DT;
        if (strcmp(prop->name, "N") == 0)
        {
            countN++;
            if (getLength(prop->values) != 5)
                return INV_PROP;
        }
        if (strcmp(prop->name, "KIND") == 0)
        {
            countKIND++;
        }
        if (getLength(prop->values) == 0)
            return INV_PROP;
        ListIterator paramIter = createIterator(prop->parameters);
        void *pData;
        while ((pData = nextElement(&paramIter)) != NULL)
        {
            Parameter *p = (Parameter *)pData;
            if (strlen(p->name) == 0 || strlen(p->value) == 0)
                return INV_PROP;
        }
    }
    if (countN > 1)
        return INV_PROP;
    if (countKIND > 1)
        return INV_PROP;

    if (obj->birthday)
    {
        if (obj->birthday->isText)
        {
            if (strlen(obj->birthday->date) > 0 || strlen(obj->birthday->time) > 0 || obj->birthday->UTC)
                return INV_DT;
        }
        else
        {
            int dateEmpty = (strlen(obj->birthday->date) == 0);
            int timeEmpty = (strlen(obj->birthday->time) == 0);
            if (dateEmpty && timeEmpty)
                return INV_DT;
            if (strlen(obj->birthday->text) > 0)
                return INV_DT;
        }
    }
    if (obj->anniversary)
    {
        if (obj->anniversary->isText)
        {
            if (strlen(obj->anniversary->date) > 0 || strlen(obj->anniversary->time) > 0 || obj->anniversary->UTC)
                return INV_DT;
        }
        else
        {
            int dateEmpty = (strlen(obj->anniversary->date) == 0);
            int timeEmpty = (strlen(obj->anniversary->time) == 0);
            if (dateEmpty && timeEmpty)
                return INV_DT;
            if (strlen(obj->anniversary->text) > 0)
                return INV_DT;
        }
    }

    return OK;
}

//------------------------------
//      Existing Functions
//------------------------------

void deleteCard(Card *obj)
{
    if (!obj)
        return;
    if (obj->fn)
        deleteProperty(obj->fn);
    if (obj->optionalProperties)
    {
        clearList(obj->optionalProperties);
        free(obj->optionalProperties);
    }
    if (obj->birthday)
        deleteDate(obj->birthday);
    if (obj->anniversary)
        deleteDate(obj->anniversary);
    free(obj);
}

char *cardToString(const Card *obj)
{
    if (!obj)
        return duplicateString("null");
    char *result = malloc(1024);
    if (!result)
        return NULL;
    sprintf(result, "FN: %s\n", (char *)getFromFront(obj->fn->values));
    if (obj->birthday)
    {
        char *bdayStr = dateToString(obj->birthday);
        sprintf(result + strlen(result), "BDAY: %s\n", bdayStr);
        free(bdayStr);
    }
    if (obj->anniversary)
    {
        char *annivStr = dateToString(obj->anniversary);
        sprintf(result + strlen(result), "ANNIVERSARY: %s\n", annivStr);
        free(annivStr);
    }
    ListIterator iter = createIterator(obj->optionalProperties);
    void *data;
    while ((data = nextElement(&iter)) != NULL)
    {
        Property *prop = (Property *)data;
        char *propStr = propertyToString(prop);
        sprintf(result + strlen(result), "%s\n", propStr);
        free(propStr);
    }
    return result;
}

char *errorToString(VCardErrorCode err)
{
    switch (err)
    {
    case OK:
        return duplicateString("OK");
    case INV_FILE:
        return duplicateString("Invalid file");
    case INV_CARD:
        return duplicateString("Invalid card");
    case INV_PROP:
        return duplicateString("Invalid property");
    case INV_DT:
        return duplicateString("Invalid date-time");
    case WRITE_ERROR:
        return duplicateString("Write error");
    case OTHER_ERROR:
        return duplicateString("Other error");
    default:
        return duplicateString("Invalid error code");
    }
}

//------------------------------
//  Helper Functions for List
//------------------------------

void deleteProperty(void *toBeDeleted)
{
    Property *prop = (Property *)toBeDeleted;
    if (prop)
    {
        free(prop->name);
        free(prop->group);
        clearList(prop->parameters);
        free(prop->parameters);
        clearList(prop->values);
        free(prop->values);
        free(prop);
    }
}

int compareProperties(const void *first, const void *second)
{
    Property *p1 = (Property *)first;
    Property *p2 = (Property *)second;
    return strcmp(p1->name, p2->name);
}

char *propertyToString(void *prop)
{
    Property *p = (Property *)prop;
    char *result = malloc(1024);
    if (!result)
        return NULL;
    if (strlen(p->group) > 0)
        sprintf(result, "%s.%s: %s", p->group, p->name, (char *)getFromFront(p->values));
    else
        sprintf(result, "%s: %s", p->name, (char *)getFromFront(p->values));
    return result;
}

void deleteParameter(void *toBeDeleted)
{
    Parameter *param = (Parameter *)toBeDeleted;
    if (param)
    {
        free(param->name);
        free(param->value);
        free(param);
    }
}

int compareParameters(const void *first, const void *second)
{
    Parameter *p1 = (Parameter *)first;
    Parameter *p2 = (Parameter *)second;
    return strcmp(p1->name, p2->name);
}

char *parameterToString(void *param)
{
    Parameter *p = (Parameter *)param;
    char *result = malloc(256);
    if (!result)
        return NULL;
    sprintf(result, "%s=%s", p->name, p->value);
    return result;
}

void deleteValue(void *toBeDeleted)
{
    char *value = (char *)toBeDeleted;
    if (value)
        free(value);
}

int compareValues(const void *first, const void *second)
{
    char *v1 = (char *)first;
    char *v2 = (char *)second;
    return strcmp(v1, v2);
}

char *valueToString(void *val)
{
    return duplicateString((char *)val);
}

void deleteDate(void *toBeDeleted)
{
    DateTime *dt = (DateTime *)toBeDeleted;
    if (dt)
    {
        free(dt->date);
        free(dt->time);
        free(dt->text);
        free(dt);
    }
}

int compareDates(const void *first, const void *second)
{
    DateTime *d1 = (DateTime *)first;
    DateTime *d2 = (DateTime *)second;
    return strcmp(d1->date, d2->date);
}

char *dateToString(void *date)
{
    DateTime *dt = (DateTime *)date;
    char *result = malloc(256);
    if (!result)
        return NULL;
    if (dt->isText)
        sprintf(result, "%s", dt->text);
    else
        sprintf(result, "%sT%s", dt->date, dt->time);
    return result;
}
