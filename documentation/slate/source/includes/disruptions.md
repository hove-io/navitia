Disruptions
===========


In this paragraph, we will explain how the disruptions are displayed in the different APIs.

# Context

## periods

### production period of coverage data
The production period is global to a navitia coverage and is the validity period of the coverage's data.
This production period cannot exceed one year.

### publication period of a disruption
The publication period of a disruption is the period on which we want to display the disruption in navitia.

The creator of the disruption might not want the traveller to know of a disruption before a certain date (because it's too uncertain, secret, ...)
The publication period is the way to control this.

### application periods of a disruption
The application periods are the list of periods on which the disruption is active.
For example it's the actual period when railworks are done and train circulation is cut.

## Request date

Request date is the navitia call date, the default value is now.

# Summary

To sum up we display an impact if 'now' is in the publication period.

The status of the impact depends only of 'now' and is:

* 'active' if 'now' is inside an application period
* 'future' if there is an application period after 'now'
* 'past' otherwise

<table>
  <thead>
    <tr>
      <th></th>
      <th align="center" colspan="2"></th>
      <th align="center"> </th>
      <th align="center" colspan="3">Show impacts </th>
      <th align="center"> </th>
    </tr>
    <tr>
      <th>Production period </th>
      <th align="center">publication period </th>
      <th>Application period </th>
      <th align="center">Request date</th>
      <th align="center">disruptions API</th>
      <th align="center">traffic_reports & PtRef API</th>
      <th align="center">Journeys & Schedules API</th>
      <th align="center">Status</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td> </td>
      <td align="center"> </td>
      <td> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
    </tr>
    <tr>
      <td> </td>
      <td align="center"> </td>
      <td> </td>
      <td align="center">date1 </td>
      <td align="center"><strong>Yes</strong> </td>
      <td align="center">- </td>
      <td align="center">- </td>
      <td align="center">future </td>
    </tr>
    <tr>
      <td> </td>
      <td align="center"> </td>
      <td> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
    </tr>
    <tr>
      <td>* </td>
      <td align="center"> </td>
      <td> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
    </tr>
    <tr>
      <td>| </td>
      <td align="center"> </td>
      <td> </td>
      <td align="center">date2 </td>
      <td align="center"><strong>Yes</strong> </td>
      <td align="center">- </td>
      <td align="center">- </td>
      <td align="center">future </td>
    </tr>
    <tr>
      <td>| </td>
      <td align="center">* </td>
      <td> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
    </tr>
    <tr>
      <td>| </td>
      <td align="center">| </td>
      <td> </td>
      <td align="center">date3 </td>
      <td align="center"><strong>Yes</strong> </td>
      <td align="center"><strong>Yes</strong> </td>
      <td align="center"> </td>
      <td align="center">future </td>
    </tr>
    <tr>
      <td>| </td>
      <td align="center">| </td>
      <td>* </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
    </tr>
    <tr>
      <td>| </td>
      <td align="center">| </td>
      <td>| </td>
      <td align="center">date4 </td>
      <td align="center"><strong>Yes</strong> </td>
      <td align="center"><strong>Yes</strong> </td>
      <td align="center"><strong>Yes</strong> </td>
      <td align="center">active </td>
    </tr>
    <tr>
      <td>| </td>
      <td align="center">| </td>
      <td>| </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
    </tr>
    <tr>
      <td>| </td>
      <td align="center">| </td>
      <td>* </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
    </tr>
    <tr>
      <td>| </td>
      <td align="center">| </td>
      <td> </td>
      <td align="center">date5 </td>
      <td align="center"><strong>Yes</strong> </td>
      <td align="center"><strong>Yes</strong> </td>
      <td align="center"> </td>
      <td align="center">passed </td>
    </tr>
    <tr>
      <td>| </td>
      <td align="center">| </td>
      <td> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
    </tr>
    <tr>
      <td>| </td>
      <td align="center">* </td>
      <td> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
    </tr>
    <tr>
      <td>| </td>
      <td align="center"> </td>
      <td> </td>
      <td align="center">date6 </td>
      <td align="center"><strong>Yes</strong> </td>
      <td align="center">- </td>
      <td align="center">- </td>
      <td align="center">passed </td>
    </tr>
    <tr>
      <td>* </td>
      <td align="center"> </td>
      <td> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
      <td align="center"> </td>
    </tr>
    <tr>
      <td> </td>
      <td align="center"> </td>
      <td> </td>
      <td align="center">date7 </td>
      <td align="center"><strong>Yes</strong> </td>
      <td align="center">- </td>
      <td align="center">- </td>
      <td align="center">passed </td>
    </tr>
  </tbody>
</table>
