
void button_onPress(Button* button)
{
    Serial.print(button->name);
    Serial.println(" was pressed");
}

void button_onRelease(Button* button)
{
    Serial.print(button->name);
    Serial.println(" was released");
}

void button_onLongClick(Button* button)
{
    Serial.print(button->name);
    Serial.println(" was long clicked");
}

void button_onClick(Button* button)
{
    char buffer[10];
    memset(buffer, 0, sizeof(buffer));
    Serial.print(button->name);
    Serial.println(" was clicked");
    if ( strcmp("A", button->name) == 0 )
    {
        if ( keezer_target_temperature < MAX_TEMPERATURE )
        {
            keezer_target_temperature++;
        }
    }
    else if ( strcmp("B", button->name) == 0 )
    {
        keezer_target_temperature = DEFAULT_TEMPERATURE;
    }
    else if ( strcmp("C", button->name) == 0 )
    {
        if ( keezer_target_temperature > MIN_TEMPERATURE )
        {
            keezer_target_temperature--;
        }
    }
    else
    {
        Serial.print(button->name);
        Serial.println(" was clicked and not handled");
    }
}
