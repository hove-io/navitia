#include "vptranslator/vptranslator.h"

void vptranslator::initcibles(){
}

MakeTranslation::MakeTranslation(){
}

bool MakeTranslation::initcs(boost::gregorian::date beginningday, std::string requestedcs){
  std::size_t found;

  found = requestedcs.find_last_of('1');
  //on recherche le dernier jour de validité de la condition de service
  if(found==std::string::npos){
    return false;
  } else {
      enddate = beginningday + boost::gregorian::date_duration(found);
      //on recherche le premier jour de validité
      found = requestedcs.find('1');
      startdate = beginningday + boost::gregorian::date_duration(found);
      CS = requestedcs.substr(startdate.day() - beginningday.day(), enddate.day() - startdate.day() + 1);
      return true;
 }
}

int MakeTranslation::getnextmonday(boost::gregorian::date datetocompare, short sens){
    if (sens == 1){
        return (8 - datetocompare.day_of_week()) % 7;
    } else {
        return ((6 + datetocompare.day_of_week()) % 7) * -1;
    }
}


void MakeTranslation::splitcs(){
//    startdate.day()

}
