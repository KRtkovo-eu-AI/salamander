Feature: About dialog
  In order to verify that the Salamander application displays product information
  As a Salamander user
  I want to open the About dialog and close it again

  Scenario: User can open and close the About dialog
    Given Salamander is running
    When I open the About dialog from the Help menu
    Then the About dialog is displayed
    When I close the About dialog
    Then the About dialog is closed
    And Salamander remains running
    When I exit Salamander
    Then Salamander is not running
