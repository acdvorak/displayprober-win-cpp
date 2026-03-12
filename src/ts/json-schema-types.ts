import * as tsj from 'ts-json-schema-generator';

interface DescProps {
  description?: string;
  markdownDescription?: string;
  fullDescription?: string;
}

export type Def = tsj.Definition & DescProps;
