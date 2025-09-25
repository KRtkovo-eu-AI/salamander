Feature: About dialog
  In order to verify that the Salamander application starts correctly
  As a Salamander power-user
  I want to open and close the About dialog from an automated test

  Scenario: Launch the application and inspect the About dialog
    Given the Salamander application is started
    When I open the About dialog
    Then the About dialog is displayed
    When I close the About dialog
    And I exit the Salamander application
    Then the Salamander application is closed
